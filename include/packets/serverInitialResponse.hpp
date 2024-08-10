#ifndef PACKETS_SERVERINITIALRESPONSE_HPP
#define PACKETS_SERVERINITIALRESPONSE_HPP

#include "packets.hpp"

namespace Packets {

class _ServerInitialResponse {
public:
    uint32_t majorVersion;
    uint32_t minorVersion;
    uint8_t playerId;

    inline _ServerInitialResponse() {}

    inline _ServerInitialResponse(uint32_t majorVersion, uint32_t minorVersion, uint8_t id) : 
        majorVersion(majorVersion), minorVersion(minorVersion), playerId(id) {}

    NetReturn netWriteToBuffer(void *buffer, uint32_t len) const;
    static NetReturn netReadFromBuffer(Packet<_ServerInitialResponse> *out, const void *buffer, uint32_t len);

    uint32_t getSize() const;

    static constexpr Tag tag = Tag::SERVER_INITIAL_RESPONSE;
    
};

typedef Packet<_ServerInitialResponse> ServerInitialResponse;

}

#endif
