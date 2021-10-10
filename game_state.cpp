#include <cmath>

#include "game_state.h"
#include "player.h"

namespace board {

    bool board_exceeded(pixel_t &pixel, server_params_t &params) {
        bool x_on_board = pixel.first < params.width;
        bool y_on_board = pixel.second < params.height;
        return !(x_on_board && y_on_board);
    }

    pixel_t get_pixel(double x, double y) {
        return {floor(x), floor(y)};
    }
}

void GameState::new_round(server_params_t &params, RandomGenerator &generator) {
    if (phase == BREAK) {
        // Starts a new game if all active players (al least 2) pressed arrow key.
        if (players_key_pushed.size() > 1 &&
            players_key_pushed.size() == active_players.size()) {
            new_game(generator, params);
        }
        return;
    }

    std::vector<client_identity_t> died;
    for (const auto& identity : current_players) {
        // Moves player's worm.
        auto all_players_it = all_players.find(identity);
        turn_direction_t player_direction = all_players_it->second.get_turn_direction();
        if (player_direction == RIGHT) {
            worm_position[identity].direction += params.turning_speed;
        }
        else if (player_direction == LEFT) {
            worm_position[identity].direction -= (-360 + params.turning_speed);
        }
        worm_position[identity].direction = worm_position[identity].direction % 360;

        pixel_t old_pixel = board::get_pixel(worm_position[identity].x, worm_position[identity].y);
        make_move(identity);
        pixel_t new_pixel = board::get_pixel(worm_position[identity].x, worm_position[identity].y);
        player_number_t player_number = all_players_it->second.get_player_number();

        // Worm stays in the same pixel.
        if (new_pixel == old_pixel)
            continue;

        // Worm moves into eaten pixel or exceeds a board.
        if (eaten_pixels.find(new_pixel) != eaten_pixels.end() ||
            board::board_exceeded(new_pixel, params)) {
            generate_player_eliminated(player_number);
            worm_position.erase(identity);

            died.push_back(identity);
            if (worm_position.size() == 1) {
                game_over();
                break;
            }
        }
        else {
            generate_pixel(player_number, new_pixel);
            eaten_pixels.insert(new_pixel);
        }

    }

    // Deletes players that lost a game from [current_players] collection.
    IdentityComparator cmp;
    for (auto &id: died) {
        auto it = current_players.begin();
        while (it != current_players.end()) {
            if (!(cmp(*it, id)) && !(cmp(id, *it))) {
                current_players.erase(it);
                break;
            }
            ++it;
        }
    }
}

void GameState::change_pressed_key(client_identity_t &identity, turn_direction_t &turn_direction) {
    auto it = all_players.find(identity);
    if (phase == GAME) {
        if (it != all_players.end()) {
            it->second.set_turn_direction(turn_direction);
        }
    }
    // Game did not start yet.
    else {
        if (it == all_players.end())
            return;

        it->second.set_turn_direction(turn_direction);
        // Memorizes that given poll_fds pressed key arrow.
        if (it->second.get_player_type() == ACTIVE &&
                (turn_direction == RIGHT || turn_direction == LEFT) &&
                players_key_pushed.find(identity) == players_key_pushed.end()) {
            players_key_pushed.insert(identity);
        }

    }
}

void GameState::change_player(client_identity_t &identity, client_message &message) {
    delete_player(identity);
    add_new_player(identity, message);
}

void GameState::delete_player(client_identity_t &identity) {
    player_name_t name;
    player_category type;

    auto it_all = all_players.find(identity);
    if (it_all != all_players.end()) {
        type = it_all->second.get_player_type();
        // Gets player's name.
        if (type == ACTIVE)
            name = it_all->second.get_player_name();

        // During the game player cannot be erased from [all_players].
        if (type == ACTIVE && phase == GAME)
            it_all->second.set_disconnected();
        else
            all_players.erase(it_all);

        if (type == ACTIVE) {
            auto it_active = active_players.find(name);
            if (it_active != active_players.end()) {
                active_players.erase(it_active);
            }
        }
    }

    // Arrow key pressed by deleted player cannot be count.
    if (phase == BREAK) {
        auto key_pushed_it = players_key_pushed.find(identity);
        if (key_pushed_it != players_key_pushed.end()) {
            players_key_pushed.erase(key_pushed_it);
        }

    }
}

void GameState::add_new_player(client_identity_t &identity, client_message &message) {
    // Add spectator.
    if (message.player_name.empty()) {
        Player player{message.session_id, message.turn_direction};
        all_players.insert({identity, player});
    }
    // Add active player.
    else {
        Player player{message.session_id, message.player_name, message.turn_direction};
        all_players.insert({identity, player});
        active_players.insert({message.player_name, identity});
        if (message.turn_direction == LEFT || message.turn_direction == RIGHT)
            players_key_pushed.insert(identity);
    }
}

void GameState::new_game(RandomGenerator &generator, server_params_t &params) {
    events.clear();
    players_key_pushed.clear();
    phase = GAME;

    game_id = generator.rand();
    player_number_t player_number = 0;
    generate_new_game(params);
    for (const auto& el : active_players) {
        auto identity = el.second;
        // Generates player's initial position.
        worm_position_t position{};
        position.x = double(generator.rand() % params.width) + 0.5;
        position.y = double(generator.rand() % params.height) + 0.5;
        position.direction = uint32_t(generator.rand() % 360);

        pixel_t pixel = board::get_pixel(position.x, position.y);
        auto it = all_players.find(identity);

        // Player's worm starts in a pixel that is already occupied.
        if (eaten_pixels.find(pixel) != eaten_pixels.end()) {
            generate_player_eliminated(player_number);
        }
        else {
            generate_pixel(player_number, pixel);
            worm_position.insert({identity, position});
            eaten_pixels.insert(pixel);
        }

        it->second.set_player_number(player_number);
        ++player_number;
        current_players.push_back(identity);
    }

    if (worm_position.size() == 1)
        game_over();
}

void GameState::game_over() {
    generate_game_over();
    worm_position.clear();
    eaten_pixels.clear();
    players_key_pushed.clear();
    current_players.clear();
    phase = BREAK;

    // Delete all disconnected players from [all_players].
    std::vector<client_identity_t> disconnected;
    for (const auto& [id, player] : all_players) {
        if (player.get_player_type() == DISCONNECTED)
            disconnected.push_back(id);
    }
    for (auto id: disconnected)
        all_players.erase(id);
}

void GameState::make_move(client_identity_t identity) {
    double direction = worm_position[identity].direction;
    worm_position[identity].x += cos(direction * M_PI / 180.0);
    worm_position[identity].y += sin(direction * M_PI / 180.0);
}

void GameState::generate_new_game(server_params_t &params) {
    event_no_t event_no = events.get_size();
    new_game_data_t data{};
    data.maxx = params.width;
    data.maxy = params.height;

    for (const auto& el : active_players) {
        auto name = el.first;
        data.players_list.push_back(name);
    }

    events.add_event(std::make_shared<NewGameEvent>(event_no, data));
}

void GameState::generate_pixel(player_number_t number, pixel_t pixel) {
    event_no_t event_no = events.get_size();
    pixel_data_t data{};
    data.player_number = number;
    data.x = pixel.first;
    data.y = pixel.second;

    events.add_event(std::make_shared<PixelEvent>(event_no, data));
}

void GameState::generate_player_eliminated(player_number_t number) {
    event_no_t event_no = events.get_size();
    player_eliminated_data_t data{};
    data.player_number = number;

    events.add_event(std::make_shared<PlayerEliminatedEvent>(event_no, data));
}

void GameState::generate_game_over() {
    event_no_t event_no = events.get_size();
    events.add_event(std::make_shared<GameOverEvent>(event_no));
}