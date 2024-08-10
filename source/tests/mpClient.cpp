#include "packets/connect.hpp"
#include "packets/serverInitialResponse.hpp"
#include "packets/playerPosition.hpp"
#include "netCommon.hpp"

extern "C" {

#include <poll.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

}

const char *SERVER_ADDR = "127.0.0.1";
uint16_t serverPort = 5000;

int main() {
    Vec position, velocity, direction;
    uint8_t recvId;
    uint8_t timer = 0;
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
    Packets::PlayerPosition pos;
    pos.position = {0.0f, 0.0f, 0.0f};
    pos.velocity = {0.0f, 0.0f, 0.0f};
    pos.direction = {0.0f, 0.0f, 0.0f};
    while(!quit) {

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
            if(ntohl(*(const uint32_t *)buffer) != (uint32_t)Packets::Tag::PLAYER_POSITION) {
                printf("Deteced unexpected packet: %d\n", *(const uint32_t *)buffer);
                continue;
            }
            Packets::PlayerPosition pos;
            NetReturn res = Packets::PlayerPosition::netReadFromBuffer(&pos, buffer + 4, amtRead - 4);
            if(res.errorCode != NetReturn::OK) {
                fprintf(stderr, "(main) Failed to read (%d, %d)\n", res.errorCode, amtRead);
                continue; 
            }
            position = pos.position;
            velocity = pos.velocity;
            direction = pos.direction;
            recvId = pos.playerId;
            if(timer == 0) {
                timer = 60;
                printf("%d: [%f %f %f] [%f %f %f] [%f %f %f]\n", recvId, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z, direction.x, direction.y, direction.z);
            }
            else timer--;
            
        }

    }

    return 0;
}
