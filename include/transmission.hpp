#ifndef TRANSMISSION_HPP
#define TRANSMISSION_HPP

#include "netCommon.hpp"

extern "C" {
    #include <netinet/ip.h>
}

namespace Transmission {

struct Connection {
    bool isActive;
    bool isCandidate;

    sockaddr_in addr;
};

class ConnectionHolder {
    Connection *connections;
    uint8_t len;
public:
    ConnectionHolder(Connection *connections, uint8_t len);
    // Either return the id of the corresponding address, or
    // create a connection candidate and return that id, along with
    // error code NetReturn::CANDIDATE. Maybe in the future return
    // FILTERED if necessary
    NetReturn getId(sockaddr_in *addr);

    inline bool isCandidate(uint8_t id) const {
        return id < len ? connections[id].isCandidate : false;
    }
    
    inline void purgeCandidate(uint8_t candidateId) {
        if(candidateId < len) connections[candidateId].isCandidate = false;
    }
    inline void purgeConnection(uint8_t id) {
        if(id < len) connections[id].isActive = false;
    }
    inline NetReturn addConnection(sockaddr_in *addr) {
        NetReturn res = getId(addr);
        if(res.errorCode == NetReturn::OK) return res;
        if(res.errorCode == NetReturn::CANDIDATE) {
            addConnection(res.bytes);
            return {res.bytes, NetReturn::OK};
        }
    }
    inline bool addConnection(uint8_t id) {
        if(id < len && connections[id].isCandidate) {
            connections[id].isCandidate = false;
            connections[id].isActive = true;
            return true;
        }
        return false;
    }

    inline const Connection* cbegin() const {return connections;}
    inline const Connection* cend() const {return connections + len;}
    
};

class Writer {
    int socket;

    const ConnectionHolder *holder;

public:
    
    inline Writer(int socket, const ConnectionHolder *holder) 
        : socket(socket), holder(holder) {}

    NetReturn write(const void *data, uint32_t size);
};

class Reader {
    int socket;

    ConnectionHolder *holder;

public:
    
    inline Reader(int socket, ConnectionHolder *holder) 
        : socket(socket), holder(holder) {}
    
    NetReturn read(void *data, uint32_t size, uint8_t *outputId);
};

}

#endif
