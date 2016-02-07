#pragma once
#include <pebble.h>

#define TOGGLE 0
#define COLOR 1
#define DIMM 2
#define DURATION_OFF 3
#define SATURATION 4
#define DURATION_ON 5
#define DURATION_ON_MS 6
#define DURATION_OFF_MS 7
#define LIFX_STATE_POWER 10
#define LIFX_STATE_BRIGHTNESS 11
#define REQUEST_STATE 20

uint8_t get_lfx_state_power();
void set_lfx_state_power(uint8_t v);

void send(int key, int value);