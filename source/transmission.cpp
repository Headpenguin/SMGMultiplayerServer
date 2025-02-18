#include "transmission.hpp"

#include <cerrno>
#include <cstring>

extern "C" {

#include <sys/socket.h>
#include <netinet/ip.h>

}

namespace Transmission {
ConnectionHolder::ConnectionHolder(Connection *connections, uint8_t len)
    : connections(connections), len(len) 
{
    for(uint8_t i = 0; i < len; i++) {
        connections[i].isActive = false;
        connections[i].isCandidate = false;
        memset(&connections[i].addr, 0, sizeof connections[i]);
    }
}

NetReturn ConnectionHolder::getId(sockaddr_in *addr) {
    Connection *i = connections, *firstFree = nullptr;
    for(; i < cend(); i++) {
        if (
            (i->isActive || i->isCandidate)
            && i->addr.sin_addr.s_addr == addr->sin_addr.s_addr
            && i->addr.sin_port == addr->sin_port
        ) {
            if(i->isActive) {
                return {static_cast<uint32_t>(i - cbegin()), NetReturn::OK};
            }
            else if(i->isCandidate) {
                return {static_cast<uint32_t>(i - cbegin()), NetReturn::CANDIDATE};
            }
        }
        else if(!firstFree && !i->isCandidate && !i->isActive) firstFree = i;
    }
    if(firstFree != nullptr) {
        firstFree->isCandidate = true;
        firstFree->addr = *addr;
        return {static_cast<uint32_t>(firstFree - cbegin()), NetReturn::CANDIDATE};
    }
    return {0, NetReturn::FILTERED};
}

NetReturn Writer::write(const void *data, uint32_t size, uint8_t destination) {
    ssize_t written;
    if(destination & 0x80 && destination != 0xFF) {
        const Connection *c = holder->getConnection(destination & 0x7F);
        if(c) {
            do {

                written = sendto(socket, data, size, 0, 
                    reinterpret_cast<const sockaddr *>(&c->addr), sizeof c->addr);

                if(written < 0) {
                    written = errno;
                }
            }
            while (written == -EAGAIN);
        }
        return {size, NetReturn::OK};
    }
    for(auto i = holder->cbegin(); i < holder->cend(); i++) {
        
        uint8_t id = holder->getId(i).bytes;

        if(!i->isActive || id == destination) continue;


        written = sendto(socket, data, size, 0, 
            reinterpret_cast<const sockaddr *>(&i->addr), sizeof i->addr);

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
    socklen_t addrlen = sizeof addr;
    
    while(read == -EAGAIN) {
        read = recvfrom(socket, data, size, 0, 
            reinterpret_cast<sockaddr *>(&addr), &addrlen);
        
        if(read < 0) read = errno;
    }

    if(read < 0) {
        return {static_cast<uint32_t>(-read), NetReturn::SYSTEM_ERROR};
    }

    NetReturn res = holder->getId(&addr);
    switch(res.errorCode) {
        case NetReturn::OK:
            *outputId = 
                static_cast<std::remove_reference<decltype(*outputId)>::type>(res.bytes);
            return {static_cast<uint32_t>(read), NetReturn::OK};
        
        case NetReturn::CANDIDATE:
            *outputId = 
                static_cast<std::remove_reference<decltype(*outputId)>::type>(res.bytes);
            return {static_cast<uint32_t>(read), NetReturn::CANDIDATE};
        
        case NetReturn::FILTERED:
            return {0, NetReturn::FILTERED};
        case NetReturn::NOT_ENOUGH_SPACE:
            return {0, NetReturn::NOT_ENOUGH_SPACE};
        default:
            return netHandleInvalidState();
    }

}

}

