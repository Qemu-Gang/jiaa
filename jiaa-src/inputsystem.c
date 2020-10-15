#include "inputsystem.h"

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <zconf.h>

bool pressedKeys[500];

#define EV_KEY			0x01

// This struct is the same at <linux/input.h>
struct input_value {
    uint16_t type;
    uint16_t code;
    int32_t value;
};

void* RunInputSystem(void *someArgs){
    int fd;
    struct input_value input;

    bool *isMainRunning = (bool*)someArgs;

    fd = open("/dev/input/evdev-mirror", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        printf("Error opening evdev-mirror! (%d), %d\nInput will be disabled.\n", fd, errno);
        return 0;
    }
    printf("Started input loop\n");
    while (*isMainRunning) {
        // if zero bytes, keep goin...
        if (!read(fd, &input, sizeof(struct input_value))) {
            usleep(1000);
            continue;
        }
        if( input.type != EV_KEY ){
            continue;
        }
        pressedKeys[input.code] = input.value > 0;
        usleep(1000);
    }
    return 0;
}