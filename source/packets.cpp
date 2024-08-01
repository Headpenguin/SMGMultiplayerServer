#include "packets/connect.hpp"

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
    implementation::Connect *packet = buffer;
    if(len < sizeof *packet) return {sizeof *packet, ErrorCode::NOT_ENOUGH_SPACE};

    memcpy(&packet->magic, &magic, sizeof packet->magic);
    packet->majorVersion = htonl(majorVersion);
    packet->minorVersion = htonl(minorVersion);

    return {sizeof *packet, ErrorCode::OK};
}

NetReturn _Connect::netReadFromBuffer(Packet<_Connect> *out, const void *buffer, uint32_t len) {
    implementation::Connect *packet = buffer;
    if(len < sizeof *packet) return {sizeof *packet, ErrorCode::NOT_ENOUGH_SPACE};

    if(*(uint32_t*)&packet->magic != htonl(CONNECT_MAGIC_UPPER)
            || *(uint32_t*)&packet->magic + 1 != htonl(CONNECT_MAGIC_LOWER)) {
        return {0, ErrorCode::INVALID_DATA};
    }

    out->majorVersion = ntohl(packet->majorVersion);
    out->minorVersion = ntohl(packet->minorVersion);

    return {sizeof *packet, ErrorCode::OK};

}

NetReturn _Ack::netWriteToBuffer(void *buffer, uint32_t len) const {
    implementation::Ack *packet = buffer;
    if(len < sizeof *packet) return {sizeof *packet, ErrorCode::NOT_ENOUGH_SPACE};

    packet->seqNum = htonl(seqNum);
    return {sizeof *packet, ErrorCode::OK};
}

NetReturn _Ack::netReadFromBuffer(Packet<_Ack> *out, const void *buffer, uint32_t len) {
    implementation::Ack *packet = buffer;
    if(len < sizeof *packet) return {sizeof *packet, ErrorCode::NOT_ENOUGH_SPACE};

    out->seqNum = ntohl(packet->seqNum);
    return {sizeof *packet, ErrorCode::OK};
}

