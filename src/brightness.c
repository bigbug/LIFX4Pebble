#include <pebble.h>
#include "brightness.h"
#include "comm.h"

static Window *s_window_brightness;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_layer;

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return 4;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  switch(cell_index->row) {
    case 0:
      menu_cell_basic_draw(ctx, cell_layer, "100 %", NULL, NULL);
      break;
    case 1:
      menu_cell_basic_draw(ctx, cell_layer, "70%", NULL, NULL);
      break;
    case 2:
      menu_cell_basic_draw(ctx, cell_layer, "50%", NULL, NULL);
      break;
    case 3:
      menu_cell_basic_draw(ctx, cell_layer, "20%", NULL, NULL);
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
  switch(cell_index->row) {
    case 0:
      send(DIMM, 100);
      break;
    case 1:
      send(DIMM, 70);
      break;
    case 2:
      send(DIMM, 50);
      break;
    case 3:
      send(DIMM, 20);
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
  
  s_status_layer = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_layer, GColorWhite, GColorBlack);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);

  window_destroy(window);
  s_window_brightness = NULL;
}

void brightness_layer_window_push() {
  if(!s_window_brightness) {
    s_window_brightness = window_create();
    window_set_background_color(s_window_brightness, PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite));
    window_set_window_handlers(s_window_brightness, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload
    });
  }
  window_stack_push(s_window_brightness, true);
}

