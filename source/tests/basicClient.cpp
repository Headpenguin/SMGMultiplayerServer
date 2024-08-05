#include "packets/connect.hpp"
#include "packets/ack.hpp"
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
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if(fd < 0) return -1;

    in_addr saddr;
    inet_aton(SERVER_ADDR, &saddr);
    sockaddr_in addr;
    addr.sin_port = htons(serverPort);
    addr.sin_family = AF_INET;
    addr.sin_addr = saddr;

    Packets::Connect connect(0, 0);
    uint8_t *buffer = (uint8_t*)aligned_alloc(4, 32);
   
    bool quit = false;

    pollfd pfd = {fd, POLLIN};
    while(!quit) {

        *(Packets::Tag*)buffer = Packets::Tag::CONNECT;
//        buffer = alignUp(buffer, );
        NetReturn res = connect.netWriteToBuffer(buffer + 4, 28);
        sendto(fd, buffer, 4 + res.bytes, 0, (sockaddr*)&addr, sizeof addr);
        
        poll(&pfd, 1, 2000);
        if(!(pfd.revents & POLLIN)) {
            fprintf(stderr, "Uh oh\n");
            continue;
        }
        pfd.events = POLLIN;

        read(fd, buffer, 2 * sizeof(Packets::Connect));
        if(ntohl(*(uint32_t*)buffer) != (uint32_t)Packets::Tag::ACK) {
            fprintf(stderr, "Wrong response!\n");
            continue;
        }
        else {
            printf("Success!\n");
            return 0;
        }


    }

    return 0;
}
