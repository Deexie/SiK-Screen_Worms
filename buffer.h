#ifndef SCREEN_WORMS_BUFFER_H
#define SCREEN_WORMS_BUFFER_H

#include <type_traits>
#include <cstdio>
#include <zconf.h>
#include <string>
#include <sys/socket.h>
#include <regex>
#include <netinet/in.h>
#include <cassert>
#include <queue>

#include "client_message.h"
#include "server_types.h"

#define DATAGRAM_SIZE 550

const std::regex player_name_regex(R"([\x21-\x7E]{0,20})");

class Buffer {
public:
    Buffer() : length(0) {}

    size_t get_space_left() const {
        return DATAGRAM_SIZE - length;
    }

    void set_destination(const struct sockaddr_in6 &destination, socklen_t len) {
        dest_addr = destination;
        addr_len = len;
    }

    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    void insert_number(const T &n) {
        T num = n;
        assert(length + sizeof(num) <= DATAGRAM_SIZE);

        switch (sizeof(num)) {
            case 2:
                num = htobe16(num);
                break;
            case 4:
                num = htobe32(num);
                break;
            case 8:
                num = htobe64(num);
                break;
            default:
                break;
        }

        memcpy(buf + length, &num, sizeof(num));
        length += sizeof(num);
    }


    void insert_string(const std::string &string);

    /*
     * Receives message from poll_fds and saves its address to [client_address].
     * Returns number of received bytes.
     */
    ssize_t receive(int sock, struct sockaddr_in6 &client_address,
        socklen_t &client_address_len);

    /*
     * Parses poll_fds message to [message].
     * Returns [true] is message was valid and [false] otherwise.
     */
    bool parse_client_message(client_message &message, ssize_t len);

    /*
     * Sends its content to given poll_fds.
     * Returns [true] if message was properly sent.
     */
    bool send_to_client(int sock);

    uint32_t get_crc32(size_t len);

    uint32_t get_crc32();

private:
    char buf[DATAGRAM_SIZE];
    size_t length;
    struct sockaddr_in6 dest_addr;
    socklen_t addr_len;
};


#endif //SCREEN_WORMS_BUFFER_H
