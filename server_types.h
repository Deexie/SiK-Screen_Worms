#ifndef SCREEN_WORMS_SERVER_TYPES_H
#define SCREEN_WORMS_SERVER_TYPES_H

#include <netinet/in.h>
#include <string.h>

using coordinate_t = uint32_t;
using pixel_t = std::pair<coordinate_t, coordinate_t>;
using game_id_t = uint32_t;
using turn_direction_t = uint8_t;
using client_identity_t = std::pair<struct in6_addr, in_port_t>;
using player_number_t = uint8_t;
using round_counter_t = uint64_t;

struct IdentityComparator {
    using is_transparent = void;

    bool operator()(const client_identity_t &id1, const client_identity_t &id2) const {
        if (id1.second == id2.second) {
            return memcmp(id1.first.s6_addr, id2.first.s6_addr, 16);
        }

        return id1.second < id2.second;
    }
};

struct server_params_t {
    int port;
    time_t generator_seed;
    int turning_speed;
    round_counter_t rounds_per_second;
    coordinate_t width;
    coordinate_t height;
};

struct worm_position_t {
    double x;
    double y;
    uint32_t direction;
};

enum direction {
    STRAIGHT = 0,
    RIGHT,
    LEFT
};

enum player_category {
    ACTIVE,
    SPECTATOR,
    DISCONNECTED
};

enum game_phase {
    GAME,
    BREAK
};

#endif //SCREEN_WORMS_SERVER_TYPES_H
