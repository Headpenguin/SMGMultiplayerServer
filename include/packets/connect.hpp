#ifndef PACKETS_CONNECT_HPP
#define PACKETS_CONNECT_HPP

#include "packets.hpp"

namespace Packets {

class _Connect {
public:
    uint32_t majorVersion;
    uint32_t minorVersion;

    inline _Connect() {}

    inline _Connect(uint32_t majorVersion, uint32_t minorVersion) : 
        majorVersion(majorVersion), minorVersion(minorVersion) {}

    NetReturn netWriteToBuffer(void *buffer, uint32_t len) const;
    static NetReturn netReadFromBuffer(Packet<_Connect> *out, const void *buffer, uint32_t len);

    uint32_t getSize() const;

    static constexpr Tag tag = Tag::CONNECT;
    
};

typedef Packet<_Connect> Connect;

}

#endif
