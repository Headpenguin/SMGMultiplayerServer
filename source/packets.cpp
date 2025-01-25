#include "packets/connect.hpp"
#include "packets/ack.hpp"
#include "packets/serverInitialResponse.hpp"
#include "packets/playerPosition.hpp"
#include "timestamps.hpp"
#include "packets/timeSync.hpp"

#include <cstring>

extern "C" {
#include <arpa/inet.h>
}

namespace Packets {

const static uint64_t CONNECT_MAGIC = 0x436F6E6E65637400;
const static uint32_t CONNECT_MAGIC_LOWER = CONNECT_MAGIC & 0xFFFFFFFF;
const static uint32_t CONNECT_MAGIC_UPPER = CONNECT_MAGIC >> 32;


namespace implementation {

    template<typename T>
    class PacketTimestamp {
        uint32_t tMs; // Big endian
    public:
        PacketTimestamp(const ClockboundTimestamp<T> &t) : 
            tMs(htonl(std::bit_cast<uint32_t>(t.t.timeMs))) {}
        ClockboundTimestamp<T> toHL() const { 
            return {std::bit_cast<int32_t>(ntohl(tMs))}; 
        }
    };

    typedef PacketTimestamp<ServerClockTag> ServerPacketTimestamp;

    class ReliablePacket {
    protected:
        uint32_t seqNum; // Big endian
    public:
        inline ReliablePacket(const ReliablePacketCode &check) : seqNum(htonl(check.seqNum)) {}
        inline ReliablePacketCode toCode() const {return ReliablePacketCode(ntohl(seqNum));}
    };

    struct Connect {
        uint8_t magic[8];
        uint32_t majorVersion; // Big endian
        uint32_t minorVersion; // Big endian
    };

    struct Ack {
        // In case of overflow, just don't accept new packets until all
        // prior packets have been accepted.
        uint32_t seqNum;
    };

    struct PlayerPosition {
        uint8_t playerId;
        uint8_t stateFlags;
        uint8_t padding[2];

        ServerPacketTimestamp timestamp;

        uint32_t positionX;
        uint32_t positionY;
        uint32_t positionZ;

        uint32_t velocityX;
        uint32_t velocityY;
        uint32_t velocityZ;

        uint32_t directionX;
        uint32_t directionY;
        uint32_t directionZ;

        uint32_t currentAnimation;
        uint32_t defaultAnimation;
        uint32_t animationSpeed;
        
    };

    struct ServerInitialResponse {
        uint32_t majorVersion;
        uint32_t minorVersion;
        uint8_t playerId;
    };

    struct TimeQuery {
        uint32_t timeMs; // Big Endian
        ReliablePacket check;
    };

    struct TimeResponse {
        uint32_t timeMs; // Big Endian
        ReliablePacket check;
    };

}

NetReturn _Connect::netWriteToBuffer(void *buffer, uint32_t len) const {
    auto *packet = reinterpret_cast<implementation::Connect *>(buffer);
    
    static_assert(std::is_layout_compatible<
        std::remove_reference<decltype(*packet)>::type, 
        implementation::Connect
    >());
    
    if(len < sizeof *packet) return {sizeof *packet, NetReturn::NOT_ENOUGH_SPACE};

    *(uint32_t *)&packet->magic = htonl(CONNECT_MAGIC_UPPER);
    *(uint32_t *)&packet->magic[4] = htonl(CONNECT_MAGIC_LOWER);
    packet->majorVersion = htonl(majorVersion);
    packet->minorVersion = htonl(minorVersion);

    return {sizeof *packet, NetReturn::OK};
}

NetReturn _Connect::netReadFromBuffer(Packet<_Connect> *out, const void *buffer, uint32_t len) {
    const auto *packet = reinterpret_cast<const implementation::Connect*>(buffer);
    
    static_assert(std::is_layout_compatible<
        std::remove_reference<decltype(*packet)>::type, 
        implementation::Connect
    >());
    
    if(len < sizeof *packet) return {sizeof *packet, NetReturn::NOT_ENOUGH_SPACE};

    if(*(uint32_t*)&packet->magic != htonl(CONNECT_MAGIC_UPPER)
            || *(uint32_t*)&packet->magic[4] != htonl(CONNECT_MAGIC_LOWER)) {
        return {0, NetReturn::INVALID_DATA};
    }

    out->majorVersion = ntohl(packet->majorVersion);
    out->minorVersion = ntohl(packet->minorVersion);

    // Remember to update getSize if the size changes
    return {sizeof *packet, NetReturn::OK};

}

uint32_t _Connect::getSize() const {
    return sizeof(implementation::Connect);
}

NetReturn _Ack::netWriteToBuffer(void *buffer, uint32_t len) const {
    auto *packet = reinterpret_cast<implementation::Ack*>(buffer);
    
    static_assert(std::is_layout_compatible<
        std::remove_reference<decltype(*packet)>::type,
        implementation::Ack
    >());
    
    if(len < sizeof *packet) return {sizeof *packet, NetReturn::NOT_ENOUGH_SPACE};

    packet->seqNum = htonl(seqNum);

    // Remember to update getSize if the size changes
    return {sizeof *packet, NetReturn::OK};
}

NetReturn _Ack::netReadFromBuffer(Packet<_Ack> *out, const void *buffer, uint32_t len) {
    const auto *packet = reinterpret_cast<const implementation::Ack*>(buffer);
    
    static_assert(std::is_layout_compatible<
        std::remove_reference<decltype(*packet)>::type,
        implementation::Ack
    >());

    if(len < sizeof *packet) return {sizeof *packet, NetReturn::NOT_ENOUGH_SPACE};

    out->seqNum = ntohl(packet->seqNum);
    return {sizeof *packet, NetReturn::OK};
}

uint32_t _Ack::getSize() const {
    return sizeof(implementation::Ack);
}

NetReturn _ServerInitialResponse::netWriteToBuffer(void *buffer, uint32_t len) const {
    auto *packet = reinterpret_cast<implementation::ServerInitialResponse *>(buffer);
    
    static_assert(std::is_layout_compatible<
        std::remove_reference<decltype(*packet)>::type, 
        implementation::ServerInitialResponse
    >());
    
    if(len < sizeof *packet) return {sizeof *packet, NetReturn::NOT_ENOUGH_SPACE};

    packet->majorVersion = htonl(majorVersion);
    packet->minorVersion = htonl(minorVersion);
    packet->playerId = playerId;

    return {sizeof *packet, NetReturn::OK};
}

NetReturn _ServerInitialResponse::netReadFromBuffer(Packet<_ServerInitialResponse> *out, const void *buffer, uint32_t len) {
    const auto *packet = reinterpret_cast<const implementation::ServerInitialResponse*>(buffer);
    
    static_assert(std::is_layout_compatible<
        std::remove_reference<decltype(*packet)>::type, 
        implementation::ServerInitialResponse
    >());
    
    if(len < sizeof *packet) return {sizeof *packet, NetReturn::NOT_ENOUGH_SPACE};

    out->majorVersion = ntohl(packet->majorVersion);
    out->minorVersion = ntohl(packet->minorVersion);
    out->playerId = packet->playerId;

    // Remember to update getSize if the size changes
    return {sizeof *packet, NetReturn::OK};

}

uint32_t _ServerInitialResponse::getSize() const {
    return sizeof(implementation::ServerInitialResponse);
}

NetReturn _PlayerPosition::netWriteToBuffer(void *buffer, uint32_t len) const {
    auto *packet = reinterpret_cast<implementation::PlayerPosition*>(buffer);
    
    static_assert(std::is_layout_compatible<
        std::remove_reference<decltype(*packet)>::type,
        implementation::PlayerPosition
    >());
    
    if(len < sizeof *packet) return {sizeof *packet, NetReturn::NOT_ENOUGH_SPACE};

    packet->playerId = playerId;
    packet->stateFlags = stateFlags;
    packet->padding[0] = 0;
    packet->padding[1] = 0;

    packet->timestamp = implementation::ServerPacketTimestamp(timestamp);

    packet->positionX = htonl(std::bit_cast<uint32_t>(position.x));
    packet->positionY = htonl(std::bit_cast<uint32_t>(position.y));
    packet->positionZ = htonl(std::bit_cast<uint32_t>(position.z));

    packet->velocityX = htonl(std::bit_cast<uint32_t>(velocity.x));
    packet->velocityY = htonl(std::bit_cast<uint32_t>(velocity.y));
    packet->velocityZ = htonl(std::bit_cast<uint32_t>(velocity.z));

    packet->directionX = htonl(std::bit_cast<uint32_t>(direction.x));
    packet->directionY = htonl(std::bit_cast<uint32_t>(direction.y));
    packet->directionZ = htonl(std::bit_cast<uint32_t>(direction.z));

    packet->currentAnimation = htonl(std::bit_cast<uint32_t>(currentAnimation));
    packet->defaultAnimation = htonl(std::bit_cast<uint32_t>(defaultAnimation));
    packet->animationSpeed = htonl(std::bit_cast<uint32_t>(animationSpeed));

    // Remember to update getSize if the size changes
    return {sizeof *packet, NetReturn::OK};
}

NetReturn _PlayerPosition::netReadFromBuffer(Packet<_PlayerPosition> *out, const void *buffer, uint32_t len) {
    const auto *packet = reinterpret_cast<const implementation::PlayerPosition*>(buffer);
    
    static_assert(std::is_layout_compatible<
        std::remove_reference<decltype(*packet)>::type,
        implementation::PlayerPosition
    >());

    if(len < sizeof *packet) return {sizeof *packet, NetReturn::NOT_ENOUGH_SPACE};

    out->playerId = packet->playerId;

    out->stateFlags = packet->stateFlags;
    
    out->timestamp = packet->timestamp.toHL();
    
    out->position = {
        std::bit_cast<float>(ntohl(packet->positionX)),
        std::bit_cast<float>(ntohl(packet->positionY)),
        std::bit_cast<float>(ntohl(packet->positionZ))
    };

    out->velocity = {
        std::bit_cast<float>(ntohl(packet->velocityX)),
        std::bit_cast<float>(ntohl(packet->velocityY)),
        std::bit_cast<float>(ntohl(packet->velocityZ))
    };
    
    out->direction = {
        std::bit_cast<float>(ntohl(packet->directionX)),
        std::bit_cast<float>(ntohl(packet->directionY)),
        std::bit_cast<float>(ntohl(packet->directionZ))
    };

    out->currentAnimation = std::bit_cast<int32_t>(ntohl(packet->currentAnimation));
    out->defaultAnimation = std::bit_cast<int32_t>(ntohl(packet->defaultAnimation));
    out->animationSpeed = std::bit_cast<float>(ntohl(packet->animationSpeed));
    

    return {sizeof *packet, NetReturn::OK};
}

uint32_t _PlayerPosition::getSize() const {
    return sizeof(implementation::PlayerPosition);
}

NetReturn _TimeQuery::netWriteToBuffer(void *buffer, uint32_t len) const {
    return {0xDEAD, NetReturn::SYSTEM_ERROR};
}

NetReturn _TimeQuery::netReadFromBuffer(TimeQuery *out, const void *buffer, uint32_t len) {
    const auto *packet = reinterpret_cast<const implementation::TimeQuery*>(buffer);
    
    static_assert(std::is_layout_compatible<
        std::remove_reference<decltype(*packet)>::type,
        implementation::TimeQuery
    >());

    if(len < sizeof *packet) return {sizeof *packet, NetReturn::NOT_ENOUGH_SPACE};

    out->timeMs = ntohl(packet->timeMs);
    out->check = packet->check.toCode();

    return {sizeof *packet, NetReturn::OK};
}

uint32_t _TimeQuery::getSize() const {
    return sizeof(implementation::TimeQuery);
}

NetReturn _TimeResponse::netWriteToBuffer(void *buffer, uint32_t len) const {
    auto *packet = reinterpret_cast<implementation::TimeResponse*>(buffer);
    
    static_assert(std::is_layout_compatible<
        std::remove_reference<decltype(*packet)>::type,
        implementation::TimeResponse
    >());
    
    if(len < sizeof *packet) return {sizeof *packet, NetReturn::NOT_ENOUGH_SPACE};

    packet->timeMs = htonl(timeMs);
    packet->check = implementation::ReliablePacket(check);

    return {sizeof *packet, NetReturn::OK};
}

NetReturn _TimeResponse::netReadFromBuffer(TimeResponse *, const void *, uint32_t) {
    return {0, NetReturn::INVALID_DATA};
}

uint32_t _TimeResponse::getSize() const {
    return sizeof(implementation::TimeResponse);
}

}

