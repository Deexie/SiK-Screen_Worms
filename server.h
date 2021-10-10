#ifndef SCREEN_WORMS_SERVER_H
#define SCREEN_WORMS_SERVER_H

#include <pthread.h>
#include <unordered_map>
#include <queue>
#include <poll.h>

#include "server_types.h"
#include "game_state.h"
#include "random_generator.h"

#define POLL_SIZE   2
#define CLIENTS_COUNT   25

using client_identity_t = std::pair<struct in6_addr, in_port_t>;

struct client_stats_t {
    session_id_t session_id;
    round_counter_t round_counter;
    struct sockaddr_in6 address;
    socklen_t address_len;
};

class Server {
public:
    Server(server_params_t &p);

    [[noreturn]] void run();

private:

    /*
     * Reads message from client and parses it.
     * Returns [true] on success and [false] otherwise.
     */
    bool get_client_message(client_message &message, client_identity_t &identity);

    /*
     * Sends answer to client message.
     * If message cannot be sent, it is pushed into waiting messages queue.
     */
    void send_answer(client_message &message, client_identity_t &identity);

    /*
     * Sends datagrams with events that were not sent yet to all clients.
     * If messages cannot be sent, they are pushed into waiting messages queue.
     */
    void broadcast_messages();


    /*
     * Disconnects all the clients who did not send_to_client any message during last
     * two seconds.
     */
    void check_timeout();

private:
    RandomGenerator generator;
    server_params_t params;
    GameState game_state;
    struct pollfd poll_fds[2];
    Buffer buffer;
    std::map<client_identity_t, client_stats_t, IdentityComparator> stats;
    std::queue<Buffer> waiting_messages;
    round_counter_t round_counter;
};


#endif //SCREEN_WORMS_SERVER_H
