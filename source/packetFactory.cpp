#include "packetFactory.hpp"
#include "packets/connect.hpp"
#include "packets/ack.hpp"

namespace Packets {

NetReturn PacketFactory::constructPacket(Tag tag, PacketUnion *pu, 
    const void *buffer, uint32_t len) const
{
    if(isCandidateMode && tag != Tag::CONNECT) return {0, NetReturn::FILTERED};
    if(tag >= Packets::Tag::MAX_TAG) return {0, NetReturn::INVALID_DATA};
    pu->tag = tag;
    switch(tag) {
        case Tag::CONNECT:
            return Connect::netReadFromBuffer(&pu->connect, buffer, len);
        case Tag::ACK:
            return Ack::netReadFromBuffer(&pu->ack, buffer, len);

        case Tag::MAX_TAG: // unreachable
            break;
    }
    return {}; // unreachable
}

}
