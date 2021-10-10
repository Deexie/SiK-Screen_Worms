#include <iostream>
#include <getopt.h>
#include "server_types.h"
#include "server.h"

#define MIN_SCREEN_SIZE 16
#define MAX_SCREEN_SIZE 4096

void fill_with_default_values(server_params_t *p) {
    p->port = 2021;
    p->turning_speed = 6;
    p->rounds_per_second = 50;
    p->width = 640;
    p->height = 480;
}

void get_options(server_params_t *p, int argc, char *argv[]) {
    bool seed_set = false;
    int opt;

    fill_with_default_values(p);
    while ((opt = getopt(argc, argv, "p:s:t:v:w:h:")) != -1) {
        switch (opt) {
            case 'p':
                p->port = strtol(optarg, nullptr, 10);
                if (errno != 0 || p->port < 1 || p->port > UINT16_MAX)
                    exit(EXIT_FAILURE);
                break;
            case 's':
                seed_set = true;
                p->generator_seed = strtol(optarg, nullptr, 10);
                if (errno != 0 || p->generator_seed > UINT32_MAX)
                    exit(EXIT_FAILURE);
                break;
            case 't':
                p->turning_speed = strtol(optarg, nullptr, 10);
                if (errno != 0 || p->turning_speed < 1 || p->turning_speed > 90)
                    exit(EXIT_FAILURE);
                break;
            case 'v':
                p->rounds_per_second = strtol(optarg, nullptr, 10);
                if (errno != 0 || p->rounds_per_second < 1 || p->rounds_per_second > 250)
                    exit(EXIT_FAILURE);
                break;
            case 'w':
                p->width = strtol(optarg, nullptr, 10);
                if (errno != 0 || p->width < MIN_SCREEN_SIZE || p->width > MAX_SCREEN_SIZE)
                    exit(EXIT_FAILURE);
                break;
            case 'h':
                p->height = strtol(optarg, nullptr, 10);
                if (errno != 0 || p->height < MIN_SCREEN_SIZE || p->height > MAX_SCREEN_SIZE)
                    exit(EXIT_FAILURE);
                break;
            default:
                fprintf(stderr, "Usage: %s [-p n] [-s n] [-t n] [-v n] [-w n] [-h n]\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!seed_set) {
        p->generator_seed = time(nullptr);
        if (p->generator_seed == -1)
            exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    server_params_t p;

    get_options(&p, argc, argv);
    Server server{p};
    server.run();

    return 0;
}

