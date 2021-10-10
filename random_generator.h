#ifndef SCREEN_WORMS_RANDOM_GENERATOR_H
#define SCREEN_WORMS_RANDOM_GENERATOR_H

#include <ctime>
#include <cstdint>

class RandomGenerator {
public:
    RandomGenerator(time_t seed) : next_value(seed) {}

    time_t rand() {
        time_t ret = next_value;
        int64_t next = (int64_t(next_value) * 279410273LL) % 4294967291LL;
        next_value = next;
        return ret;
    }

private:
    time_t next_value;
};

#endif //SCREEN_WORMS_RANDOM_GENERATOR_H
