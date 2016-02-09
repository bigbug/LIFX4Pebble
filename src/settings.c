#include <pebble.h>
#include "settings.h"
#include "mainMenu.h"
#include "comm.h"
#include "alarm.h"
#include "wakeup.h"

static Window *s_main_window;
static MenuLayer *s_menu_layer;
static char lightup_text[20];
static char flash_text[30];
static StatusBarLayer *s_status_layer;

void settings_window_mark_dirty() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Main Menu DIRTY!");
  //if(s_menu_layer)
  layer_mark_dirty((Layer *)s_menu_layer);
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  window_destroy(s_main_window);
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return 4;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  switch(cell_index->row) {
    case 0:
      snprintf(lightup_text, sizeof(lightup_text), "Light up %d min",alarm_get()->secondsToLightUpBeforeAlarm/60);
      menu_cell_basic_draw(ctx, cell_layer, lightup_text, "before alarm goes of", NULL);
      break;
    case 1:
      snprintf(flash_text, sizeof(flash_text), "Snooze to flash: %d",alarm_get()->flashingAfterXSnoozes);
      menu_cell_basic_draw(ctx, cell_layer, flash_text, "Light starts pulsating", NULL);
      break;
    case 2:
      menu_cell_basic_draw(ctx, cell_layer, "Test Alarm", "Immediately fires an alarm", NULL);
      break;
    case 3:
      menu_cell_basic_draw(ctx, cell_layer, "Test Alarm in 10 s", NULL, NULL);
      break;
    default:
      break;
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return
    menu_layer_is_index_selected(menu_layer, cell_index) ?
      MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT : MENU_CELL_ROUND_UNFOCUSED_TALL_CELL_HEIGHT;
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  time_t future_time = time(NULL) + 10;
  
  //Alarm *alarm = alarm_get();
  switch(cell_index->row) {
    case 0:
      if(alarm_get()->secondsToLightUpBeforeAlarm==60) {alarm_get()->secondsToLightUpBeforeAlarm=120;}
      else if(alarm_get()->secondsToLightUpBeforeAlarm==120) {alarm_get()->secondsToLightUpBeforeAlarm=300;}
      else if(alarm_get()->secondsToLightUpBeforeAlarm==300) {alarm_get()->secondsToLightUpBeforeAlarm=600;}
      else if(alarm_get()->secondsToLightUpBeforeAlarm==600) {alarm_get()->secondsToLightUpBeforeAlarm=900;}
      else if(alarm_get()->secondsToLightUpBeforeAlarm==900) {alarm_get()->secondsToLightUpBeforeAlarm=1800;}
      else {alarm_get()->secondsToLightUpBeforeAlarm=60;}
      break;
    case 1:
      alarm_get()->flashingAfterXSnoozes = (alarm_get()->flashingAfterXSnoozes + 1) % 5;
      break;
    case 2:
      wakeup_launch_window(0);
      break;
    case 3:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Scheduled wakeup timer");
      wakeup_schedule(future_time, WAKEUP_REASON_ALARM, true);
      break;
    default:
      break;
  }
  settings_window_mark_dirty();
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
  
  s_status_layer = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_layer, GColorWhite, GColorBlack);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_layer));
}

void settings_window_init(void) {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
  });
  window_stack_push(s_main_window, true);
}