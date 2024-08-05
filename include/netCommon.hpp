#ifndef NETCOMMON_HPP
#define NETCOMMON_HPP

#include <stdexcept>
#include <cstdint>

struct NetReturn {
    
    enum ErrorCode : unsigned char {
        OK = 0,
        NOT_ENOUGH_SPACE,
        INVALID_DATA,
        INVALID_STATE,
        CANDIDATE,
        FILTERED,
        SYSTEM_ERROR
    };
    
    uint32_t bytes;
    ErrorCode errorCode;
};

template<typename T>
inline constexpr T alignUp(T base, size_t alignment) {
    T tmp = base + alignment - 1;
    return tmp - (size_t)tmp % alignment;
}

template<typename T>
inline constexpr T alignDown(T base, size_t alignment) {
    return base - (size_t)base % alignment;
}

inline NetReturn netHandleInvalidState() {
#ifdef DEBUG
    throw std::logic_error("Invalid state");
#endif
    return {0, NetReturn::INVALID_STATE};
}

#endif
