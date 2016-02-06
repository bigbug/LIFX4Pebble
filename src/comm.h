#pragma once
#include <pebble.h>

#define TOGGLE 0
#define COLOR 1
#define DIMM 2
#define DURATION_OFF 3
#define LIFX_STATE_POWER 10
#define LIFX_STATE_BRIGHTNESS 11
#define REQUEST_STATE 20

void send(int key, int value);