#include "transmission.hpp"

#include <cerrno>

extern "C" {

#include <sys/socket.h>
#include <netinet/ip.h>

}

namespace Transmission {

NetReturn ConnectionHolder::getId(sockaddr_in *addr) {
    Connection *i = connections, *firstFree = nullptr;
    for(; i < cend(); i++) {
        if(i->isActive) return {i - cbegin(), NetReturn::OK};
        if(!firstFree && !i->isCandidate) firstFree = i;
    }
    if(firstFree != nullptr) return {firstFree, NetReturn::CANDIDATE};
    return {0, NetReturn::NOTENOUGHSPACE};
}

NetReturn Writer::write(const void *data, uint32_t size) {
    ssize_t written;
    for(auto i = holder->cbegin(); i < holder->cend();) {
        
        if(!i->isActive) continue;

        written = sendto(socket, data, size, 0, &i->addr, sizeof i->addr);       
        i++;
        if(written < 0) {
            written = errno;
            if(written == -EAGAIN) i--;
        }
    }
    return {size, NetReturn::OK};
}

NetReturn Reader::read(void *data, uint32_t size, uint8_t *outputId) {
    ssize_t read = -EAGAIN;
    sockaddr_in addr;
    
    while(read == -EAGAIN) {
        read = recvfrom(socket, data, size, 0, &addr, sizeof addr);
        if(read < 0) read = errno;
    }

    if(read < 0) {
        return {read, NetReturn::SYSTEM_ERROR};
    }

    NetReturn res = holder->getId(&addr);
    switch(res.errorCode) {
        case NetReturn::OK:
            *outputId = res.id;
            return {read, NetReturn::OK};
        
        case NetReturn::CANDIDATE:
            *outputId = res.id;
            return {read, NetReturn::CANDIDATE};
        
        case NetReturn::FILTERED:
            return {0, NetReturn::FILTERED};
        case NetReturn::NOTENOUGHSPACE:
            return {0, NetReturn::NOTENOUGHSPACE};
        default:
            return netHandleInvalidState();
    }

}

}

