#ifndef NETCOMMON_HPP
#define NETCOMMON_HPP

struct NetReturn {
    
    enum ErrorCode : unsigned char {
        OK = 0,
        NOT_ENOUGH_SPACE,
        INVALID_DATA,
        INVALID_STATE
    };
    
    uint32_t bytes;
    ErrorCode errorCode;
};

template<typename T, typename U>
inline constexpr T alignUp(T base, U alignment) {
    T tmp = base + alignment - 1;
    return tmp - tmp % alignment;
}

template<typename T, typename U>
inline constexpr T alignDown(T base, U alignment) {
    return base - base % alignment;
}

NetReturn netHandleInvalidState() {
#ifdef DEBUG
    throw std::logic_error("Invalid state");
#endif
    return {0, NetReturn::INVALID_STATE};
}

#endif
