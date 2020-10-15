#ifndef JIAA_INPUTSYSTEM_H
#define JIAA_INPUTSYSTEM_H

#include <stdbool.h>
#include <linux/input-event-codes.h> // use these for pressedKeys array

// Requires evdev-mirror: https://github.com/LWSS/evdev-mirror
// keyboard is 0-256 and mouse is > 256, so let's make the array unreasonably big to avoid overwriting other data
extern bool pressedKeys[500];

void* RunInputSystem(void *someArgs);

#endif //JIAA_INPUTSYSTEM_H
