#include "packetFactory.hpp"

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
        case Tag::SERVER_INITIAL_RESPONSE:
            return ServerInitialResponse::netReadFromBuffer(&pu->sip, buffer, len);
        case Tag::PLAYER_POSITION:
            return PlayerPosition::netReadFromBuffer(&pu->playerPos, buffer, len);
        case Tag::TIME_QUERY:
            return TimeQuery::netReadFromBuffer(&pu->timeQuery, buffer, len);
        case Tag::TIME_RESPONSE:
            return TimeResponse::netReadFromBuffer(&pu->timeResponse, buffer, len);
        case Tag::MAX_TAG: // unreachable
            break;
    }
    return {}; // unreachable
}

}
