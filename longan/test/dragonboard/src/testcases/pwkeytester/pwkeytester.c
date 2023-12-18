#include <stdio.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "include/types.h"

#define INPUT_DIR	"/dev/input/"

__s32 main(__s32 argc, char **argv)
{
    __s32 ret = 0, count = 0;
    __s32 fd;
    struct input_event ev;
    char devname[256];
    const char s[2] = "=";
    char *token;

    token = strtok(argv[1], s);
    if (token != NULL) {
        token = strtok(NULL, s);
        strcpy(devname, INPUT_DIR);
        strcat(devname, token);
    } else {
        strcpy(devname, argv[1]);
    }

    fd = open(devname, O_RDONLY|O_NONBLOCK);
    if (fd < 0) {
        printf("open the input devices err!!!:%s\n", devname);
        return 1;
    }

    printf("plese pull down the power key!\n");
    fflush(NULL);

    while (1) {
        ret = read(fd, &ev, sizeof(struct input_event));
        if (ret > 0) {
            printf("power key success!\n");
            return 0;
        }
        fflush(NULL);

        sleep(1);
    }

    printf("read event error, power key fail!\n");
    return 1;
}
