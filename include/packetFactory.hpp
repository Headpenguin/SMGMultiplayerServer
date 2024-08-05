#ifndef PACKETFACTORY_HPP
#define PACKETFACTORY_HPP

#include "packets.hpp"
#include "packets/connect.hpp"
#include "packets/ack.hpp"

namespace Packets {

class PacketFactory {
    bool isCandidateMode;
public:

    struct PacketUnion {
        Tag tag;
        union {
            Connect connect;
            Ack ack;
        };
        PacketUnion() {}
    };

    NetReturn constructPacket(Tag tag, PacketUnion *pu, const void *buffer, uint32_t len) const;
    inline void setCandidate() {isCandidateMode = true;}
    inline void resetCandidate() {isCandidateMode = false;}
};

}

#endif
