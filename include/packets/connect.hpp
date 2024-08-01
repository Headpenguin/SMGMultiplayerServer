#ifndef PACKETS_CONNECT_HPP
#define PACKETS_CONNECT_HPP

#include "packets.hpp"

namespace Packets {

class _Connect {
public:
    uint8_t Magic[8];
    uint32_t majorVersion;
    uint32_t minorVersion;
};

}

#endif
