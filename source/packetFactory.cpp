#include "packetFactory.hpp"
#include "packets/connect.hpp"
#include "packets/ack.hpp"

namespace Packets {

NetReturn PacketFactory::constructPacket(Tag tag, PacketUnion *pu, 
    const void *buffer, uint32_t len) 
{
    if(isCandidateMode && tag != CONNECT) return {0, NetReturn::FILTERED};
    pu->tag = tag;
    switch(tag) {
        case CONNECT:
            return Connect::netReadFromBuffer(&pu->connect, buffer, len);
        case ACK:
            return Ack::netReadFromBuffer(&pu->ack, buffer, len);
    }
}

}
