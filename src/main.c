#include <pebble.h>
#include "comm.h"
#include "brightness.h"
#include "turnOffOver.h"
#include "color.h"


static Window *s_main_window;
//static TextLayer *s_output_layer;

//static DictationSession *s_dictation_session;
//static char s_last_text[512];
static MenuLayer *s_menu_layer;
static uint8_t lfx_state_power = 20;
//static uint8_t lfx_state_brightness;

static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
  // Ready for next command
  //text_layer_set_text(s_output_layer, "Press up or down.");
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  //text_layer_set_text(s_output_layer, "Send failed!");
  APP_LOG(APP_LOG_LEVEL_ERROR, "Fail reason: %d", (int)reason);
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return 4;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  switch(cell_index->row) {
    case 0:
      if(lfx_state_power==20) {
        menu_cell_basic_draw(ctx, cell_layer, "Connecting ...", NULL, NULL);
      } else if(lfx_state_power>0) {
        menu_cell_basic_draw(ctx, cell_layer, "Turn off", NULL, NULL);
      } else {
        menu_cell_basic_draw(ctx, cell_layer, "Turn on", NULL, NULL);
      }
      break;
    case 1:
      menu_cell_basic_draw(ctx, cell_layer, "Turn off over", NULL, NULL);
      break;
    case 2:
      menu_cell_basic_draw(ctx, cell_layer, "Brightness", NULL, NULL);
      break;
    case 3:
      menu_cell_basic_draw(ctx, cell_layer, "Color", NULL, NULL);
      break;
    default:
      break;
      menu_cell_basic_draw(ctx, cell_layer, "Toggle", NULL, NULL);
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return
    menu_layer_is_index_selected(menu_layer, cell_index) ?
      MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT : MENU_CELL_ROUND_UNFOCUSED_TALL_CELL_HEIGHT;
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  switch(cell_index->row) {
    case 0:
      if(lfx_state_power==1) {
        send(TOGGLE, 0);
      } else if(lfx_state_power==0) {
        send(TOGGLE, 1);
      } else {
        send(TOGGLE, 2);
      }
      break;
    case 1:
      turnOffOver_layer_window_push();
      break;
    case 2:
      brightness_layer_window_push();
      break;
    case 3:
      color_layer_window_push();
      break;
    /*case 1:
      send(DIMM, 1);
      break;
    case 2:
      send(DIMM, 0);
      break;
    case 3:
      send(COLOR, 1);
      break;
    case 4:
      send(COLOR, 2);
      break;
    case 5:
      send(COLOR, 3);
      break;
    case 6:
      send(TOGGLE, 0);
      break;
    case 7:
      send(DURATION_OFF, 30);
      break;*/
    default:
      break;
  }
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
  menu_layer_set_highlight_colors(s_menu_layer, GColorCyan, GColorBlack);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
      .select_click = select_callback,
  });
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}

//static char s_buffer[64];


static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received message");
  
  // Get the first pair
  Tuple *data = dict_find(iterator, LIFX_STATE_POWER);
  if (data) {
    lfx_state_power = data->value->uint8;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Power state: %d", (int)lfx_state_power);
  }
  
  layer_mark_dirty((Layer *)s_menu_layer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void init(void) {
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
  });
  window_stack_push(s_main_window, true);

  // Open AppMessage
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  // Create new dictation session
  //s_dictation_session = dictation_session_create(sizeof(s_last_text), dictation_session_callback, NULL);
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}