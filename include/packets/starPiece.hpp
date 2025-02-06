#ifndef STARPIECE_HPP
#define STARPIECE_HPP

#include "vec.hpp"
#include "timestamps.hpp"
#include "packets.hpp"

namespace Packets {

class _StarPiece {
public:
    uint8_t playerId;
    ServerTimestamp timestamp;
    Vec initLineStart;
    Vec initLineEnd;

    inline _StarPiece() : timestamp(makeEmptyServerTimestamp()) {}
    NetReturn netWriteToBuffer(void *buffer, uint32_t len) const;
    static NetReturn netReadFromBuffer(Packet<_StarPiece> *out, const void *buffer, uint32_t len);
    uint32_t getSize() const;
    static constexpr Tag tag = Tag::STAR_PIECE;
};

typedef Packet<_StarPiece> StarPiece;

}

#endif
