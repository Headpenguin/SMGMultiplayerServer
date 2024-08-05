#include "packets/connect.hpp"
#include "packets/ack.hpp"

#include <cstring>

extern "C" {
#include <arpa/inet.h>
}

namespace Packets {

const static uint64_t CONNECT_MAGIC = 0x436F6E6E65637400;
const static uint32_t CONNECT_MAGIC_LOWER = CONNECT_MAGIC & 0xFFFFFFFF;
const static uint32_t CONNECT_MAGIC_UPPER = CONNECT_MAGIC >> 32;


namespace implementation {

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

}

