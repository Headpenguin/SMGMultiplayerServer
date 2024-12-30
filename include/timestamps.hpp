#ifndef TIMESTAMPS_HPP
#define TIMESTAMPS_HPP

struct Timestamp {
    uint32_t timeMs;
};

template<typename T>
struct ClockboundTimestamp {
    Timestamp t;
};

struct ServerClockTag {};

typedef ClockboundTimestamp<ServerClockTag> ServerTimestamp;

#endif
