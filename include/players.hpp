#ifndef PLAYERS_HPP
#define PLAYERS_HPP

#include "vec.hpp"

namespace Player {

class Player {
    bool active;
    Vec position;
    Vec velocity;
    Vec direction;
public:
    inline void updateInfo(const Vec *_position, const Vec *_velocity, const Vec *_direction) {
        active = true;
        if(_position) position = *_position;
        if(_velocity) velocity = *_velocity;
        if(_direction) direction = *_direction;
    }
    inline Vec getPosition() const {return position;}
    inline Vec getVelocity() const {return velocity;}
    inline Vec getDirection() const {return direction;}
    inline bool isActive() const {return active;}
    inline void deactivate() {active = false;}
};

}

#endif
