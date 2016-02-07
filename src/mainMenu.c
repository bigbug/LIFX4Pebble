#include <pebble.h>
#include "mainMenu.h"
#include "comm.h"
#include "brightness.h"
#include "turnOffOver.h"
#include "color.h"
#include "saturation.h"
#include "progress_layer_window.h"
#include "win_edit.h"
#include "alarm.h"
#include "wakeup.h"

static Window *s_main_window;
static MenuLayer *s_menu_layer;

void main_window_mark_dirty() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Main Menu DIRTY!");
  //if(s_menu_layer)
    layer_mark_dirty((Layer *)s_menu_layer);
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  window_destroy(s_main_window);
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return 8;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  switch(cell_index->row) {
    case 0:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Current power status: %d", get_lfx_state_power());
      if(get_lfx_state_power()==20) {
        menu_cell_basic_draw(ctx, cell_layer, "Getting status ...", NULL, NULL);
      } else if(get_lfx_state_power()>0) {
        menu_cell_basic_draw(ctx, cell_layer, "Turn off", NULL, NULL);
      } else {
        menu_cell_basic_draw(ctx, cell_layer, "Turn on", NULL, NULL);
      }
      break;
    case 1:
      menu_cell_basic_draw(ctx, cell_layer, "Turn off over ...", NULL, NULL);
      break;
    case 2:
      menu_cell_basic_draw(ctx, cell_layer, "Brightness", NULL, NULL);
      break;
    case 3:
      menu_cell_basic_draw(ctx, cell_layer, "Color", NULL, NULL);
      break;
    case 4:
      menu_cell_basic_draw(ctx, cell_layer, "Saturation", NULL, NULL);
      break;
    case 5:
      menu_cell_basic_draw(ctx, cell_layer, "Alarm", NULL, NULL);
      break;
    case 6:
      menu_cell_basic_draw(ctx, cell_layer, "Test Alarm", NULL, NULL);
      break;
    case 7:
      menu_cell_basic_draw(ctx, cell_layer, "Test Alarm in 10 s", NULL, NULL);
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
  time_t future_time = time(NULL) + 10;
  //Alarm *alarm = alarm_get();
  switch(cell_index->row) {
    case 0:
      if(get_lfx_state_power()==1) {
        send(TOGGLE, 0);
      } else if(get_lfx_state_power()==0) {
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
    case 4:
      saturation_layer_window_push();
      break;
    case 5:
      //perform_wakeup_tasks();
      win_edit_init();
      win_edit_show(alarm_get());
      break;
    case 6:
      wakeup_launch_window(0);
      break;
    case 7:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Scheduled wakeup timer");
      wakeup_schedule(future_time, WAKEUP_REASON_ALARM, true);
      break;
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

void main_window_init(void) {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
  });
  window_stack_push(s_main_window, true);
}