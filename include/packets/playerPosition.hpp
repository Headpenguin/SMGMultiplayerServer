#ifndef PACKETS_PLAYERPOSITION_HPP
#define PACKETS_PLAYERPOSITION_HPP

#include "vec.hpp"

#include "timestamps.hpp"

namespace Packets {


class _PlayerPosition {
public:
    uint8_t playerId;

    ServerTimestamp timestamp;

    Vec position;
    Vec velocity;
    Vec direction;

    int32_t currentAnimation;
    int32_t defaultAnimation;
    float animationSpeed;

    enum {
        O_STATE_HIPDROP = 1
    }; // State Flags

    uint8_t stateFlags;

    inline _PlayerPosition() : timestamp(makeEmptyServerTimestamp()), stateFlags(0) {}

    NetReturn netWriteToBuffer(void *buffer, uint32_t len) const;
    static NetReturn netReadFromBuffer(Packet<_PlayerPosition> *out, const void *buffer, uint32_t len);

    uint32_t getSize() const;

    static constexpr Tag tag = Tag::PLAYER_POSITION;
    
};

typedef Packet<_PlayerPosition> PlayerPosition;

}

#endif
