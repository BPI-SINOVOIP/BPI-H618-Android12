
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "options.h"

static struct option opts[] = {
    {"size",        required_argument, 0, 's'},
    {"position",    required_argument, 0, 'p'},
    {"transform",   no_argument,       0, 't'},
    {"blackground", no_argument,       0, 'b'},
};

static void usage(const char* cmd)
{
    printf("Usage: %s [-s wxh] [-p left,top] [-t] [-b]\n"
           "       -s|--size wxh           specify buffer's size\n"
           "       -p|--position left,top  specify display frame position\n"
           "       -t| --transform         apply display's inverse transform\n"
           "                               available values:\n"
           "                                1: HAL_TRANSFORM_FLIP_H\n"
           "                                2: HAL_TRANSFORM_FLIP_V\n"
           "                                4: HAL_TRANSFORM_ROT_90\n"
           "                                3: HAL_TRANSFORM_ROT_180  (FLIP_H | FLIP_V)\n"
           "                                7: HAL_TRANSFORM_ROT_270 ((FLIP_H | FLIP_V) | ROT_90)\n"
           "       -b| --blackground       show blackground layer\n",
           cmd);
    exit(1);
}

int parse_options(int argc, char** argv, params_t* p)
{
    int c;
    const char* comm = argv[0];

    while (1) {
        c = getopt_long(argc, argv, "s:p:f:t:bh", opts, NULL);
        if (c == -1)
            break;

        switch (c) {
            case 's':
                if (sscanf(optarg, "%dx%d", &p->width, &p->height) != 2)
                    usage(comm);
                break;
            case 'p':
                if (sscanf(optarg, "%d,%d", &p->left, &p->top) != 2)
                    usage(comm);
                break;
            case 't':
                if (sscanf(optarg, "%d", &p->transform) != 1)
                    usage(comm);
                break;
            case 'b':
                p->blackground = 1;
                break;
            case 'f':
                if (sscanf(optarg, "%f", &p->scale) != 1)
                    usage(comm);
                break;
            case 'h':
            default:
                usage(comm);
                break;
        }
    }
    return 0;
}
