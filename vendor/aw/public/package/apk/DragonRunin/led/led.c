#define LOG_TAG "DragonRunin-Leds"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <log/log.h>
#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LEDR             "PF5"
#define LEDG             "PF3"
#define PIN_CTRL_ROOT    "/sys/class/gpio_sw"
#define PROP_TEST_RESULT "debug.test.result"

int node_write(char *path, char *buf, ssize_t size)
{
    int fd;
    fd = open(path, O_WRONLY);
    if (fd <= 0)
        return -1;

    if (write(fd, buf, size) < 0)
        return -1;

    close(fd);
    return 0;
}

int led_init()
{
    char buf[] = "1\n";

    if (node_write(PIN_CTRL_ROOT "/PF3/cfg", buf, strlen(buf)))
        return -1;
    if (node_write(PIN_CTRL_ROOT "/PF5/cfg", buf, strlen(buf)))
        return -1;

    return 0;
}

int led_set(char *ledname, int val)
{
    char node[256];
    char buf[8];
    sprintf(node, PIN_CTRL_ROOT "/%s/light", ledname);
    sprintf(buf, "%d\n", val);
    if (node_write(node, buf, strlen(buf)))
        return -1;
    return 0;
}

int main(void)
{
    char buf[PROPERTY_VALUE_MAX];
    int state = 0;
    int count = 0;
    while (1) {
        if (property_get(PROP_TEST_RESULT, buf, NULL) == 0)
            state = 0;
        else
            state = buf[0] - 0x30;

        ALOGD("%s: %s, state: %d, run time: %ds", PROP_TEST_RESULT, buf, state, count++);

        led_init();

        switch (state) {
            case 1:
                led_set(LEDR, 0);
                led_set(LEDG, 0);
                usleep(500000);
                led_set(LEDR, 1);
                led_set(LEDG, 1);
                usleep(500000);
                break;
            case 2:
                led_set(LEDR, 0);
                led_set(LEDG, 0);
                usleep(500000);
                led_set(LEDR, 1);
                usleep(500000);
                break;
            case 3:
                led_set(LEDR, 0);
                led_set(LEDG, 0);
                usleep(500000);
                led_set(LEDG, 1);
                usleep(500000);
                break;
            default:
                led_set(LEDR, 0);
                led_set(LEDG, 0);
                usleep(1000000);
                return 0;
        }
    }
    return 0;
}
