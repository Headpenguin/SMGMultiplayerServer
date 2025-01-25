#ifndef TIMESTAMPS_HPP
#define TIMESTAMPS_HPP

struct Timestamp {
    int32_t timeMs;
};

template<typename T>
struct ClockboundTimestamp {
    Timestamp t;
};

struct ServerClockTag {};

typedef ClockboundTimestamp<ServerClockTag> ServerTimestamp;

inline ServerTimestamp makeEmptyServerTimestamp() {
    return {std::numeric_limits<int32_t>::min()};
}

#endif
