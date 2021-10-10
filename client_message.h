#ifndef SCREEN_WORMS_CLIENT_MESSAGE_H
#define SCREEN_WORMS_CLIENT_MESSAGE_H

using session_id_t = uint64_t;
using player_name_t = std::string;
using event_no_t = uint32_t;

struct client_message {
    session_id_t session_id;
    uint8_t turn_direction;
    event_no_t next_expected_event_no;
    player_name_t player_name;
};

#endif //SCREEN_WORMS_CLIENT_MESSAGE_H
