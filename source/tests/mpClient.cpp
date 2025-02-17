#include "packets/connect.hpp"
#include "packets/serverInitialResponse.hpp"
#include "packets/playerPosition.hpp"
#include "packets/starPiece.hpp"
#include "netCommon.hpp"
#include <cmath>
#include <numbers>

extern "C" {

#include <poll.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

}

const char *SERVER_ADDR = "127.0.0.1";
uint16_t serverPort = 5029;

const static float MAX_VELOCITY = 10.0f;
const static float MAX_ACCELERATION = 1.0f;
const static float TOLERANCE = 20.0f;
const static float TOLERANCE_D1 = 0.01f;

int main() {
    Vec position, velocity, direction;
    Vec cDestination{0, 0, 0}, cPosition, cVelocity, cAcceleration;
    uint8_t recvId;
    uint32_t timer = 0;
    bool arrive = true;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if(fd < 0) return -1;

    in_addr saddr;
    inet_aton(SERVER_ADDR, &saddr);
    sockaddr_in addr;
    addr.sin_port = htons(serverPort);
    addr.sin_family = AF_INET;
    addr.sin_addr = saddr;

    Packets::Connect connect(0, 0);
    uint8_t *buffer = (uint8_t*)aligned_alloc(4, 64);
   
    bool quit = false;
    bool connected = false;
    uint8_t id = ~0;

    pollfd pfd = {fd, POLLIN};
    pollfd pfdin = {STDIN_FILENO, POLLIN};
    Packets::PlayerPosition pos;
    Packets::StarPiece piece;
    pos.position = {0.0f, 0.0f, 0.0f};
    pos.velocity = {0.0f, 0.0f, 0.0f};
    pos.direction = {1.0f, 0.0f, 0.0f};
    pos.currentAnimation = -1;
    pos.defaultAnimation = -1;
    pos.timestamp = {0};
    //pos.stateFlags |= pos.O_STATE_HIPDROP;
    int highest = -0x80000000;
    bool pposo = false;
    while(!quit) {
        poll(&pfdin, 1, 0);
        if(pfdin.revents & POLLIN) {
            char *line = nullptr;
            size_t n;
            ssize_t res = getline(&line, &n, stdin);
            if(res >= 0) {
                float x, y, z, vx, vy, vz;
                uint32_t t;
                if(sscanf(line, "(%f, %f, %f), (%f, %f, %f), %d", &x, &y, &z, &vx, &vy, &vz, &t) == 7) {
                    pos.position = {x, y, z};
                    pos.velocity = {vx, vy, vz};
                    if(t) pos.timestamp = {t};
                }
                else {
                    printf("Toggle player pos output\n");
                    pposo = !pposo;
                }
                arrive = false;
            }
            free(line);

        }
        cAcceleration = cDestination - cPosition;
        if(cAcceleration.equal(Vec::zero(), TOLERANCE)) {
            arrive = true;
            cAcceleration = Vec::zero();
        }
        else {
            cAcceleration.setLength(MAX_ACCELERATION);
        

            cVelocity += cAcceleration;
            if(cVelocity.equal(Vec::zero(), TOLERANCE_D1)) cVelocity = Vec::zero();
            else cVelocity.setLength(MAX_VELOCITY);

            cPosition += cVelocity;
            pos.position = cPosition;
        }
//            printf("(%f, %f, %f)\n", pos.position.x, pos.position.y, pos.position.z);
            
        pos.timestamp = {highest};
        *(uint32_t*)buffer = ntohl((uint32_t)(connected ? Packets::Tag::PLAYER_POSITION : Packets::Tag::CONNECT));
//        buffer = alignUp(buffer, );
        NetReturn res;
        if(!connected) res = connect.netWriteToBuffer(buffer + 4, 28);
        else res = pos.netWriteToBuffer(buffer + 4, 60);
        if(res.errorCode != NetReturn::OK) {
            fprintf(stderr, "(main) Really bad... %d\n", res.errorCode);
            continue;
        }
        sendto(fd, buffer, 4 + res.bytes, 0, (sockaddr*)&addr, sizeof addr);
        
        poll(&pfd, 1, 2000);
        if(!(pfd.revents & POLLIN)) {
            fprintf(stderr, "Uh oh\n");
            continue;
        }
        pfd.events = POLLIN;

        ssize_t amtRead = read(fd, buffer, 64);

        if(amtRead < 0) {
            perror("(main) read failed");
            continue;
        }
        if(amtRead < 4) {
            fprintf(stderr, "Received too short of response from server");
            continue;
        }
        if(!connected) {
            if(ntohl(*(const uint32_t *)buffer) != (uint32_t)Packets::Tag::SERVER_INITIAL_RESPONSE) {
                fprintf(stderr, "Received invalid response from server\n");
                continue;
            }
            Packets::ServerInitialResponse sip;
            NetReturn res = Packets::ServerInitialResponse::netReadFromBuffer(&sip, buffer + 4, amtRead - 4);
            if(res.errorCode != NetReturn::OK) {
                fprintf(stderr, "(main) Failed to read (%d, %d, %ld)\n", res.errorCode, res.bytes, amtRead);
                continue; 
            }
            id = sip.playerId;
            pos.playerId = id;
            printf("%d\n", id);
            connected = true;
        }
        else {
            if(ntohl(*(const uint32_t *)buffer) == (uint32_t)Packets::Tag::PLAYER_POSITION) {
                Packets::PlayerPosition pos;
                NetReturn res = Packets::PlayerPosition::netReadFromBuffer(&pos, buffer + 4, amtRead - 4);
                if(res.errorCode != NetReturn::OK) {
                    fprintf(stderr, "(main) Failed to read (%d, %ld)\n", res.errorCode, amtRead);
                    continue; 
                }
                position = pos.position;
                velocity = pos.velocity;
                direction = pos.direction;
                recvId = pos.playerId;
                highest = pos.timestamp.t.timeMs > highest ? pos.timestamp.t.timeMs : highest;
            }
            else if(ntohl(*(const uint32_t *)buffer) == (uint32_t)Packets::Tag::STAR_PIECE) {
                Packets::StarPiece piece;
                NetReturn res = Packets::StarPiece::netReadFromBuffer(&piece, buffer + 4, amtRead - 4);
                if(res.errorCode != NetReturn::OK) {
                    fprintf(stderr, "(main) Failed to read (%d, %ld)\n", res.errorCode, amtRead);
                    continue;
                }
                const Vec &org = piece.initLineStart;
                const Vec &dst = piece.initLineEnd;

                printf("Piece %d: [%f %f %f] [%f %f %f] %d\n", piece.playerId, org.x, org.y, org.z, dst.x, dst.y, dst.z, piece.timestamp.t.timeMs);
            }
            else {
                printf("Deteced unexpected packet: %d\n", *(const uint32_t *)buffer);
                continue;
            }
            if(timer == 0) {
                timer = 600;
            }
            timer--;
            if(timer % 60 == 0 && pposo) {
                printf("%d: [%f %f %f] [%f %f %f] [%f %f %f] %d\n", recvId, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z, direction.x, direction.y, direction.z, highest);
                printf("[%f %f %f]\n", position.x, position.y, sqrt(1 - position.x * position.x - position.y * position.y));
                printf("%d %d\n", pos.currentAnimation, pos.defaultAnimation);
                printf("%f\n", pos.animationSpeed);
            }
            if(timer % 600 == 0 && connected) {
                piece.timestamp = {highest};
                piece.initLineEnd.x = position.x;
                piece.initLineEnd.y = position.y;
                piece.initLineEnd.z = position.z;
                piece.initLineStart = piece.initLineEnd + Vec(500.0f, 0.0f, 0.0f);
                piece.playerId = id;
                *(uint32_t*)buffer = ntohl((uint32_t)Packets::Tag::STAR_PIECE);
                NetReturn res;
                res = piece.netWriteToBuffer(buffer + 4, 60);
                if(res.errorCode != NetReturn::OK) {
                    fprintf(stderr, "(main) Really bad... %d\n", res.errorCode);
                    continue;
                }
                printf("Sendto result: %ld %f %f %f\n", sendto(fd, buffer, 4 + res.bytes, 0, (sockaddr*)&addr, sizeof addr), piece.initLineEnd.x, piece.initLineEnd.y, piece.initLineEnd.z);
                for(unsigned int i = 0; i < 64; i++) {
                    printf("%x ", buffer[i]);
                }
                printf("\n");
            }
            
        }

    }

    return 0;
}
