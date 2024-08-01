#ifndef PACKETS_HPP
#define PACKETS_HPP

#include <cstddef>

namespace Packets {

	
enum ErrorCode : uint8_t {
    OK = 0,
    NOT_ENOUGH_SPACE,
    INVALID_DATA
};

struct NetReturn {
    uint32_t bytes;
    ErrorCode errorCode;
};

template<typename T>
class Packet : public T {
public:

	inline NetReturn netWriteToBuffer(void *buffer, uint32_t len) const {
        return T::netWriteToBuffer(buffer, len);
    }

    inline static NetReturn netReadFromBuffer(T *out, const void *buffer, uint32_t len) {
        return T::netReadFrombuffer(out, buffer, len);
    }
	
	
};

}

#endif
