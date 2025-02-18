#ifndef PACKETS_HPP
#define PACKETS_HPP

#include <cstdint>
#include "netCommon.hpp"

namespace Packets {

// Can never be less than 4
constexpr size_t PACKET_ALIGNMENT = 8;
// Must be power of 2
constexpr uint32_t MAX_PACKET_SIZE = 1 << 7; 

enum class Tag : uint32_t {
    CONNECT = 0,
    ACK,
    SERVER_INITIAL_RESPONSE,
    PLAYER_POSITION,
    TIME_QUERY,
    TIME_RESPONSE,
    MAX_TAG
};

namespace implementation {
    class ReliablePacket;
}

class ReliablePacketCode {
protected:
    uint32_t seqNum;
public:
    inline ReliablePacketCode() {}
    inline ReliablePacketCode(uint32_t seqNum) : seqNum(seqNum) {}

    inline bool verify(const ReliablePacketCode &other) const {
        return other.seqNum == seqNum;
    }
    friend class implementation::ReliablePacket;
};

template<typename T>
class Packet : public T {
public:

    inline Packet() = default;

    template<typename... U>
    inline Packet(const U&... args) : T(args...) {}
    
    inline uint32_t getSize() const {
        return T::getSize();
    }

	inline NetReturn netWriteToBuffer(void *buffer, uint32_t len) const {
        return T::netWriteToBuffer(buffer, len);
    }

    inline static NetReturn netReadFromBuffer(Packet<T> *out, const void *buffer, uint32_t len) {
        return T::netReadFromBuffer(out, buffer, len);
    }

};

}

#endif
