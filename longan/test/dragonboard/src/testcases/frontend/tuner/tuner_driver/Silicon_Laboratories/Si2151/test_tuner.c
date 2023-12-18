#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include "i2c.h"

#define R828_ADDRESS (0xF6 >> 1)
extern void tuner_init(void);

extern int Si2151_Test(int argc, char *argv[]);
extern void connect_test(void);

int main (int argc, char * argv[])
{
    const char* path = "/dev/i2c-2";

    printf("SI2151 tuner!\n");
    set_i2c_port(path);
    connect_test();
    Si2151_Test(0, NULL);

    return 0;
}

