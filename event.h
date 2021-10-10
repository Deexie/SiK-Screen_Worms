#ifndef SCREEN_WORMS_EVENT_H
#define SCREEN_WORMS_EVENT_H

#include <cstdint>
#include <vector>
#include "buffer.h"

using player_name_t = std::string;
using coordinate_t = uint32_t;
using player_number_t = uint8_t;
using event_no_t = uint32_t;
using event_type_t = uint8_t;

enum event_type {
    NEW_GAME,
    PIXEL,
    PLAYER_ELIMINATED,
    GAME_OVER
};

struct new_game_data_t {
    coordinate_t maxx;
    coordinate_t maxy;
    std::vector<player_name_t> players_list;
};

struct pixel_data_t {
    player_number_t player_number;
    coordinate_t x;
    coordinate_t y;
};

struct player_eliminated_data_t {
    player_number_t player_number;
};

class Event {
public:
    Event(event_no_t number, event_type_t type) : event_no(number), event_type(type),
            crc32(0), len(0), crc32_found(false) {
        len += sizeof(event_no) + sizeof(event_type);
    }

    size_t get_event_length() const {
        return len + sizeof(len) + sizeof(crc32);
    }

    void stringify(Buffer &buffer) {
        buffer.insert_number(len);
        buffer.insert_number(event_no);
        buffer.insert_number(event_type);
    }

    virtual void stringify(Buffer &buffer, bool crc) = 0;

    void set_crc(Buffer &buffer, bool crc) {
        if (crc) {
            if (!crc32_found) {
                crc32_found = true;
                Buffer buf;
                stringify(buf, false);
                crc32 = buf.get_crc32();
            }
            buffer.insert_number(crc32);
        }
    }

    virtual ~Event() = default;
protected:
    event_no_t event_no;
    event_type_t event_type;
    uint32_t crc32;
    uint32_t len;
    bool crc32_found;
};

class NewGameEvent : public Event {
public:
    NewGameEvent(event_no_t number, new_game_data_t &new_game_data) :
        Event(number, NEW_GAME), data(new_game_data) {
        len += sizeof(new_game_data.maxx) + sizeof(new_game_data.maxy);
        for (auto &name: new_game_data.players_list) {
            len += name.length() + 1;
        }
    }

    void stringify(Buffer &buffer, bool crc) override {
        Event::stringify(buffer);
        buffer.insert_number(data.maxx);
        buffer.insert_number(data.maxy);
        for (auto &name: data.players_list) {
            buffer.insert_string(name);
        }
        Event::set_crc(buffer, crc);
    }

private:
    new_game_data_t data;
};

class PixelEvent : public Event {
public:
    PixelEvent(event_no_t number, pixel_data_t pixel_data) :
            Event(number, PIXEL), data(pixel_data) {
        len += sizeof(data.x) + sizeof(data.y) + sizeof(data.player_number);
    }

    void stringify(Buffer &buffer, bool crc) override {
        Event::stringify(buffer);
        buffer.insert_number(data.player_number);
        buffer.insert_number(data.x);
        buffer.insert_number(data.y);
        Event::set_crc(buffer, crc);
    }

private:
    pixel_data_t data;
};

class PlayerEliminatedEvent : public Event {
public:
    PlayerEliminatedEvent(event_no_t number, player_eliminated_data_t eliminated_data) :
        Event(number, PLAYER_ELIMINATED), data(eliminated_data) {
        len += sizeof(data.player_number);
    }

    void stringify(Buffer &buffer, bool crc) override {
        Event::stringify(buffer);
        buffer.insert_number(data.player_number);
        Event::set_crc(buffer, crc);
    }

private:
    player_eliminated_data_t data;
};

class GameOverEvent : public Event {
public:
    GameOverEvent(event_no_t number) : Event(number, GAME_OVER) {}

    void stringify(Buffer &buffer, bool crc) override {
        Event::stringify(buffer);
        Event::set_crc(buffer, crc);
    }
};

#endif //SCREEN_WORMS_EVENT_H
