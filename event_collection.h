#ifndef SCREEN_WORMS_EVENT_COLLECTION_H
#define SCREEN_WORMS_EVENT_COLLECTION_H

#include <memory>

#include "event.h"

class EventCollection {
public:
    EventCollection() : next_for_broadcast(0) {}

    size_t get_size() const {
        return events.size();
    }
    
    size_t get_next_for_broadcast() const {
        return next_for_broadcast;
    }

    std::shared_ptr<Event> get_event(size_t index) const {
        if (index >= events.size())
            return nullptr;

        return events[index];
    }

    void all_broadcasted() {
        next_for_broadcast = events.size();
    }

    void add_event(const std::shared_ptr<Event> &event) {
        events.push_back(event);
    }

    void clear() {
        events.clear();
        next_for_broadcast = 0;
    }

private:
    std::vector<std::shared_ptr<Event>> events;
    size_t next_for_broadcast;
};


#endif //SCREEN_WORMS_EVENT_COLLECTION_H
