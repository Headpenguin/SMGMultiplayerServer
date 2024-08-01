#ifndef PACKETS_ACK_HPP
#define PACKETS_ACK_HPP

#include "packets/packets.hpp"

namespace Packets {

class _Ack {
public:
    uint32_t seqNum;

    inline _Ack() = default;
    inline _Ack(uint32_t seqNum) : seqNum(seqNum) {}
    NetReturn netWriteToBuffer(void *buffer, uint32_t len) const;
    static NetReturn netReadFromBuffer(Packet<_Ack> *out, const void *buffer, uint32_t len);
};

typedef Packet<_Ack> Ack;

}

#endif
