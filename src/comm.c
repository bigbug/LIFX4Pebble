#include <pebble.h>
#include "comm.h"

static uint8_t lfx_state_power = 20;

uint8_t get_lfx_state_power() {
  return lfx_state_power;
}

void set_lfx_state_power(uint8_t v) {
  lfx_state_power = v;
}

void send(int key, int value) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  dict_write_int(iter, key, &value, sizeof(int), true);

  app_message_outbox_send();
}