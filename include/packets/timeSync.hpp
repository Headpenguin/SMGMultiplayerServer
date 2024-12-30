#ifndef PACKETS_TIMESYNC_HPP
#define PACKETS_TIMESYNC_HPP

#include "packets.hpp"

namespace Packets {

class _TimeQuery {
public:
    uint32_t timeMs;
    ReliablePacketCode check;

    uint32_t getSize() const;
    NetReturn netWriteToBuffer(void *buffer, uint32_t len) const;
    static NetReturn netReadFromBuffer(Packet<_TimeQuery> *out, const void *buffer, uint32_t len);

    static constexpr Tag tag = Tag::TIME_QUERY;
};

class _TimeResponse {
public:
    uint32_t timeMs;
    ReliablePacketCode check;

    inline _TimeResponse(uint32_t timeMs, ReliablePacketCode check) : timeMs(timeMs), check(check) {}

    uint32_t getSize() const;
    NetReturn netWriteToBuffer(void *buffer, uint32_t len) const;
    static NetReturn netReadFromBuffer(Packet<_TimeResponse> *out, const void *buffer, uint32_t len);
    static constexpr Tag tag = Tag::TIME_RESPONSE;
};

typedef Packet<_TimeQuery> TimeQuery;
typedef Packet<_TimeResponse> TimeResponse;

}

#endif
