#include <cstdio>
#include <netinet/in.h>
#include <map>
#include <zconf.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <cmath>

#include "server_types.h"
#include "server.h"
#include "err.h"

enum poll_elems {
    SOCK = 0,
    TIMER
};

Server::Server(server_params_t &p) : generator(p.generator_seed), params(p),
        round_counter(0) {
    for (int i = 0; i < 2; ++i) {
        poll_fds[i].fd = -1;
        poll_fds[i].events = POLLIN;
        poll_fds[i].revents = 0;
    }

    int sock;
    struct sockaddr_in6 server_address;

    sock = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr("socket");

    server_address.sin6_family = AF_INET6;
    server_address.sin6_addr = in6addr_any;
    server_address.sin6_port = htons(p.port);
    if (bind(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        syserr("bind");

    poll_fds[SOCK].fd = sock;

    // Sets a sock to nonblocking mode.
    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0)
        syserr("fcntl");
}

bool Server::get_client_message(client_message &message, client_identity_t &identity) {
    struct sockaddr_in6 client_address;
    socklen_t client_address_len;

    ssize_t len = buffer.receive(poll_fds[SOCK].fd, client_address, client_address_len);
    if (len <= 0)
        return false;

    identity = {client_address.sin6_addr, client_address.sin6_port};

    // Valid data was received.
    if (buffer.parse_client_message(message, len)) {
        // Data received from already known client.
        if (stats.find(identity) != stats.end()) {
            if (stats[identity].session_id == message.session_id) {
                stats[identity].round_counter = round_counter;
                game_state.change_pressed_key(identity, message.turn_direction);
            }
            // New player connected from known address and port.
            else if (stats[identity].session_id > message.session_id &&
                    !game_state.player_name_in_use(message.player_name, identity)) {
                stats[identity].session_id = message.session_id;
                stats[identity].round_counter = round_counter;
                stats[identity].address = client_address;
                stats[identity].address_len = client_address_len;

                game_state.change_player(identity, message);
            }
            else {
                return false;
            }
        }
        // Data received from new client.
        else {
            if (stats.size() >= CLIENTS_COUNT ||
                    game_state.player_name_in_use(message.player_name))
                return false;

            client_stats_t s = {.session_id = message.session_id, .round_counter = round_counter,
                                .address = client_address, .address_len = client_address_len};
            stats.insert({identity, s});
            game_state.add_new_player(identity, message);
        }

        return true;
    }

    return false;
}

void Server::send_answer(client_message &message, client_identity_t &identity) {
    auto next_event = message.next_expected_event_no;
    while (game_state.get_events().get_size() > next_event) {
        Buffer buf;
        buf.insert_number(game_state.get_game_id());
        // Puts in datagram as many events as it can.
        while (game_state.get_events().get_event(next_event) != nullptr &&
                game_state.get_events().get_event(next_event)->get_event_length() <=
                buf.get_space_left()) {
            game_state.get_events().get_event(next_event)->stringify(buf, true);
            ++next_event;
        }

        // Tries to send_to_client datagram to given client. If it fails, buffer with datagram
        // is pushed into waiting messages queue.
        buf.set_destination(stats[identity].address, stats[identity].address_len);
        if (!waiting_messages.empty() || !buf.send_to_client(poll_fds[SOCK].fd)) {
            waiting_messages.push(buf);
        }
    }
}

void Server::broadcast_messages() {
    auto next_event = game_state.get_events().get_next_for_broadcast();
    while (game_state.get_events().get_size() > next_event) {
        Buffer buf;
        buf.insert_number(game_state.get_game_id());
        // Puts in datagram as many events as it can.
        while (game_state.get_events().get_event(next_event) != nullptr &&
                game_state.get_events().get_event(next_event)->get_event_length() <=
                buf.get_space_left()) {
            game_state.get_events().get_event(next_event)->stringify(buf, true);
            ++next_event;
        }

        // Tries to send_to_client datagram to all connected client. If it fails, buffer
        // with datagram is pushed into waiting messages queue.
        for (const auto& el : stats) {
            auto identity = el.first;
            buf.set_destination(stats[identity].address, stats[identity].address_len);
            if (!waiting_messages.empty() || !buf.send_to_client(poll_fds[SOCK].fd)) {
                waiting_messages.push(buf);
            }
        }
    }

    game_state.get_events().all_broadcasted();
}

void Server::check_timeout() {
    std::vector<client_identity_t> timeouted;

    for (auto it = stats.begin(); it != stats.end(); ++it) {
        // Recent connection was at least 2 seconds ago.
        if (round_counter - it->second.round_counter > 2 * params.rounds_per_second) {
            client_identity_t id = it->first;
            timeouted.push_back(id);

            game_state.delete_player(id);
        }
    }

    for (auto &id: timeouted) {
        stats.erase(id);
    }
}

[[noreturn]] void Server::run() {
    int ret;
    struct itimerspec round_timer;
    // Sets and arms timer.
    uint32_t miliseconds = 1000 / params.rounds_per_second;
    round_timer.it_value.tv_sec = __time_t(1.0 / double(params.rounds_per_second));
    round_timer.it_value.tv_nsec = 1000000 * (miliseconds % 1000);
    round_timer.it_interval.tv_sec = __time_t(1.0 / double(params.rounds_per_second));
    round_timer.it_interval.tv_nsec = 1000000 * (miliseconds % 1000);

    int timer_fd = timerfd_create(CLOCK_REALTIME, 0);
    if (timer_fd == -1)
        syserr("timerfd");
    if (timerfd_settime(timer_fd, 0, &round_timer, nullptr) == -1)
        syserr("timerfd_settime");
    poll_fds[TIMER].fd = timer_fd;

    while (true) {
        // Resets examined events.
        for (auto &i : poll_fds)
            i.revents = 0;

        // If some messages are to be sent, possibility of writing to socket should
        // be examined.
        poll_fds[SOCK].events = waiting_messages.empty() ? POLLIN : (POLLIN | POLLOUT);

        ret = poll(poll_fds, POLL_SIZE, -1);
        if (ret == -1) {
            if (errno == EINTR)
                fprintf(stderr, "Interrupted system call\n");
            else
                syserr("poll");

            continue;
        }

        // Round timer.
        if (poll_fds[TIMER].fd != -1 && (poll_fds[TIMER].revents & POLLIN)) {
            int64_t timer;
            auto ret = read(poll_fds[TIMER].fd, &timer, 8);
            if (ret != 8)
                exit(EXIT_FAILURE);

            ++round_counter;
            check_timeout();
            game_state.new_round(params, generator);
            broadcast_messages();
        }
        // Sends messages from [waiting_messages].
        if ((poll_fds[SOCK].revents & POLLOUT)) {
            Buffer buf = waiting_messages.front();
            waiting_messages.pop();
            if (!buf.send_to_client(poll_fds[SOCK].fd)) {
                waiting_messages.push(buf);
            }
        }
        // New clients' connections.
        if ((poll_fds[SOCK].revents & POLLIN)) {
            client_message message;
            client_identity_t identity;
            // Client message is valid.
            if (get_client_message(message, identity)) {
                send_answer(message, identity);
            }
        }
    }
}

