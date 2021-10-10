#ifndef SCREEN_WORMS_PLAYER_H
#define SCREEN_WORMS_PLAYER_H

#include "client_message.h"

class Player {
public:
    Player(session_id_t id, player_name_t &name, turn_direction_t turn_dir) : session_id(id),
            player_name(name), player_type(ACTIVE), turn_direction(turn_dir), player_number(0) {}

    Player(session_id_t id,  turn_direction_t turn_dir) : session_id(id), player_type(SPECTATOR),
            turn_direction(turn_dir), player_number(0) {}

    session_id_t get_session_id() const {
        return session_id;
    }

    player_name_t get_player_name() const {
        return player_name;
    }

    player_category get_player_type() const {
        return player_type;
    }

    turn_direction_t get_turn_direction() const {
        return turn_direction;
    }

    player_number_t get_player_number() const {
        return player_number;
    }

    void set_turn_direction(turn_direction_t dir) {
        turn_direction = dir;
    }

    void set_player_number(player_number_t num) {
        player_number = num;
    }

    void set_disconnected() {
        player_type = DISCONNECTED;
    }

private:
    session_id_t session_id;
    player_name_t player_name;
    player_category player_type;
    turn_direction_t turn_direction;
    player_number_t player_number;
};


#endif //SCREEN_WORMS_PLAYER_H
