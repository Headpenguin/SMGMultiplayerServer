#ifndef PACKETFACTORY_HPP
#define PACKETFACTORY_HPP

#include "packets.hpp"

namespace Packets {

struct PacketUnion {
    Tag tag;
    union {
        Connect connect;
        Ack ack;
    };
};

class PacketFactory {
    bool isCandidateMode;
public:
    typedef PacketUnion PacketUnion;

    NetReturn constructPacket(Tag tag, PacketUnion *pu, const void *buffer, uint32_t len);
    inline void setCandidate() {isCandidateMode = true;}
    inline void resetCandidate() {isCandidateMode = false;}
};

}

#endif
