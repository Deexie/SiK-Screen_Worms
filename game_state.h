#ifndef SCREEN_WORMS_GAME_STATE_H
#define SCREEN_WORMS_GAME_STATE_H

#include <set>
#include <map>

#include "random_generator.h"
#include "server_types.h"
#include "player.h"
#include "event_collection.h"

class GameState {
public:
    GameState() : game_id(0), phase(BREAK) {}

    game_id_t get_game_id() const {
        return game_id;
    }

    EventCollection &get_events() {
        return events;
    }

    /*
     * Checks if given player name is already in use.
     */
    bool player_name_in_use(const player_name_t &player_name) const {
        return active_players.find(player_name) != active_players.end();
    }

    /*
     * Checks if given player name is already in use by any player except for one with
     * given [identity].
     */
    bool player_name_in_use(const player_name_t &player_name,
                            const client_identity_t &identity) const {
        auto it = active_players.find(player_name);
        IdentityComparator cmp;
        return (it != active_players.end() &&
            (cmp(it->second, identity) || cmp(identity, it->second)));
    }

    /*
     * Starts a new round if called during a game.
     * If called during a break between two games (or before the first game), it checks
     * whether a new game should be started.
     */
    void new_round(server_params_t &params, RandomGenerator &generator);

    /*
     * Changes key recently pressed by given player.
     * If called during a break,
     */
    void change_pressed_key(client_identity_t &identity, turn_direction_t &turn_direction);

    void change_player(client_identity_t &identity, client_message &message);
    void delete_player(client_identity_t &identity);
    void add_new_player(client_identity_t &identity, client_message &message);

private:
    void new_game(RandomGenerator &generator, server_params_t &params);
    void game_over();

    /*
     * Moves given player by 1 in given direction.
     */
    void make_move(client_identity_t identity);

    void generate_new_game(server_params_t &params);
    void generate_pixel(player_number_t number, pixel_t pixel);
    void generate_player_eliminated(player_number_t number);
    void generate_game_over();

private:
    game_id_t game_id;
    std::map<client_identity_t, worm_position_t, IdentityComparator> worm_position;
    std::map<client_identity_t, Player, IdentityComparator> all_players;
    std::map<player_name_t, client_identity_t> active_players;
    EventCollection events;
    std::set<pixel_t> eaten_pixels;
    std::set<client_identity_t, IdentityComparator> players_key_pushed;
    std::vector<client_identity_t> current_players;
    game_phase phase;
};


#endif //SCREEN_WORMS_GAME_STATE_H
