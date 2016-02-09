#include <pebble.h>
#include "win_edit.h"

//
//  win-edit.c
//  alarms++
//
//  Created by Christian Reinbacher on 01.11.14.
//
//

#include "win_edit.h"
#include "localize.h"
#include "mainMenu.h"
#include "alarm.h"

static void time_window_load(Window* window);
static void time_window_unload(Window* window);
static void click_config_provider(void *context);

static Window *s_time_window;
static Layer *s_canvas_layer;
static TextLayer *s_input_layers[3];
static StatusBarLayer *s_time_status_layer;
static StatusBarLayer *s_status_layer;

static char s_value_buffers[3][3];
static int s_selection;
static char s_digits[3];
static char s_max[3];
static char s_min[3];
static bool s_withampm;

static char s_time_text[10];

#ifdef PBL_RECT
#define OFFSET_LEFT 0
#define OFFSET_TOP 0
#define ITEM_HEIGHT 28
#define OFFSET_ITEM_TOP 0
#else
#define OFFSET_LEFT 18
#define OFFSET_TOP 11
#define ITEM_HEIGHT 40
#define OFFSET_ITEM_TOP 6
#endif
#define PIN_WINDOW_SPACING 24
static const GPathInfo PATH_INFO = {
  .num_points = 3,
  .points = (GPoint []) {{0, -5}, {5,5}, {-5, 5}}
};
static GPath *s_my_path_ptr;

static void window_load(Window* window);
static void window_unload(Window* window);

static Window *s_window;
static MenuLayer* s_menu;
static GBitmap *check_icon,*check_icon_inv;
static bool s_is_am;
static bool s_select_all;

static char s_countdown_text[30];
static TextLayer *s_layer_countdown;

// Menu stuff
#define MENU_SECTION_WEEKDAYS 1
#define MENU_SECTION_OK 0

//static uint16_t menu_num_sections(struct MenuLayer* menu, void* callback_context);
static uint16_t menu_num_rows(struct MenuLayer* menu, uint16_t section_index, void* callback_context);
static int16_t menu_cell_height(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context);
//static int16_t menu_header_height(struct MenuLayer *menu, uint16_t section_index, void *callback_context);
static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context);
//static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* callback_context);
static void menu_select(struct MenuLayer* menu, MenuIndex* cell_index, void* callback_context);

static Alarm temp_alarm;
static Alarm *current_alarm;

void save_alarm(bool addToRing)
{
  AlarmTime tempAlarmTime;
  //temp_alarm.enabled = cell_index->row==0;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Exiting alarm window");
  if(temp_alarm.enabled) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "alarm ENABLED");
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "alarm DISABLED");
  }
  // update timer, destroy windows
  //temp_alarm.enabled=true;
  if(!clock_is_24h_style()) {
    // convert hours and am/pm back
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Hour before conversion is %d",temp_alarm.hour);
    if(s_is_am) {
      int hour = temp_alarm.hour;
      hour -= 12;
      if(hour<0) hour+=12;
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Hour after conversion is %d",hour);
      temp_alarm.hour = hour;
    } else {
      temp_alarm.hour = ((temp_alarm.hour+12)%12) + 12;
    }          
  }

  tempAlarmTime.hour = temp_alarm.hour;
  tempAlarmTime.minute = temp_alarm.minute;

  if(addToRing)
    alarm_ring_add(&tempAlarmTime);
  
  memcpy(current_alarm,&temp_alarm,sizeof(Alarm));
  alarm_process();
}

void win_edit_init(void)
{
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  
  s_time_window = window_create();
  window_set_window_handlers(s_time_window, (WindowHandlers) {
    .load = time_window_load,
    .unload = time_window_unload,
  });

  check_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK_INV);
  check_icon_inv = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK);

  s_my_path_ptr= gpath_create(&PATH_INFO);
}

void win_edit_show(Alarm *alarm){
  
  memcpy(&temp_alarm,alarm,sizeof(Alarm));
  current_alarm = alarm;
  s_select_all = false;
  s_max[2]=1;s_min[2]=0;
  s_max[1]=59;s_min[1]=0;
  int hour;
  if(clock_is_24h_style())
  {
    s_withampm=false;
    s_max[0]=23;s_min[0]=0;
  }
  else
  {
    s_withampm=true;
    s_max[0]=12;s_min[0]=1;
    convert_24_to_12(temp_alarm.hour, &hour, &s_is_am);
    temp_alarm.hour = hour;
    s_digits[2] = s_is_am;
  }
  s_selection = 0;
  if(temp_alarm.hour==0 && temp_alarm.minute==0)
  {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    convert_24_to_12(t->tm_hour, &hour, &s_is_am);
    s_digits[0] = clock_is_24h_style()?t->tm_hour:hour;
    s_digits[1] = t->tm_min;
    s_digits[2] = s_is_am;
  } else
  {
    s_digits[0] = temp_alarm.hour;
    s_digits[1] = temp_alarm.minute;
  }
  // Go to recent alarms menu
  window_stack_push(s_window, true);
  //window_stack_push(s_time_window,true);
}

// Time input window stuff

static void update_ui(Layer *layer, GContext *ctx) {
  
  for(int i = 0; i < 3; i++) {
    text_layer_set_background_color(s_input_layers[i], (i == s_selection) ? PBL_IF_COLOR_ELSE(GColorBlue,GColorBlack) : PBL_IF_COLOR_ELSE(GColorDarkGray,GColorLightGray));
    text_layer_set_text_color(s_input_layers[i],GColorWhite);
    if(i==s_selection)
    {
      GPoint selection_center = {
        .x = (int16_t) (s_withampm?23:50) + i * (PIN_WINDOW_SPACING + PIN_WINDOW_SPACING) + OFFSET_LEFT,
        .y = (int16_t) 50 + OFFSET_TOP,
      };
      gpath_rotate_to(s_my_path_ptr, 0);
      gpath_move_to(s_my_path_ptr, selection_center);
      graphics_context_set_fill_color(ctx,PBL_IF_COLOR_ELSE(GColorBlue,GColorBlack));
      gpath_draw_filled(ctx, s_my_path_ptr);
      gpath_rotate_to(s_my_path_ptr, TRIG_MAX_ANGLE/2);
      selection_center.y = 110 + OFFSET_TOP;
      gpath_move_to(s_my_path_ptr, selection_center);
      gpath_draw_filled(ctx, s_my_path_ptr);
    }
    if(i<2)
      snprintf(s_value_buffers[i], sizeof("00"), "%02d", s_digits[i]);
    else
      snprintf(s_value_buffers[i], sizeof("AM"), s_digits[i]?"AM":"PM");
    text_layer_set_text(s_input_layers[i], s_value_buffers[i]);
  }
  
  time_t tempTime = alarm_get_time_of_wakeup(&temp_alarm) - time(NULL);
  int cMin = (int)((int)tempTime / 60);
  int cHour = (int)((int)cMin / 60);
  cMin = cMin % 60;
  snprintf(s_countdown_text, sizeof(s_countdown_text), "%02d h %02d min til alarm starts", cHour, cMin);
  text_layer_set_text(s_layer_countdown, s_countdown_text);
  
  layer_set_hidden(text_layer_get_layer(s_input_layers[2]),!s_withampm);
  // draw the :

  graphics_context_set_text_color(ctx,PBL_IF_COLOR_ELSE(GColorBlue,GColorBlack));
  graphics_draw_text(ctx,":",fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),GRect(s_withampm?144/2-27:144/2 + OFFSET_LEFT,58 + OFFSET_TOP,40,20),
                     GTextOverflowModeWordWrap,GTextAlignmentLeft,NULL);
  
#ifdef PBL_ROUND
  // draw graphical representations as rings around the border
  /*GRect bounds = layer_get_bounds(layer);
  int hour_angle = (s_digits[0] * 360) / 12;
  graphics_context_set_fill_color(ctx, s_selection==0?GColorBlue:GColorDarkGray);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, 10, 0, DEG_TO_TRIGANGLE(hour_angle));
  int minute_angle = (s_digits[1] * 360) / 60;
  bounds = grect_inset(bounds,GEdgeInsets(9));
  graphics_context_set_fill_color(ctx, s_selection==1?GColorBlue:GColorDarkGray);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, 9, 0, DEG_TO_TRIGANGLE(minute_angle));*/
  
#endif
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Next column
  s_selection++;
  
  if(s_selection == (s_withampm? 3:2)) {
    temp_alarm.enabled = true;
    s_is_am = s_digits[2];
    //window_stack_push(s_window,true);
    //window_stack_push(s_time_window, true);
    s_selection--;
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "about to leave menu");
    
    //save_alarm(false);
    save_alarm(true);
    // Return to main menu
    window_stack_pop(true);
    window_stack_pop(false);
  }
  else
    layer_mark_dirty(s_canvas_layer);
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Previous column
  s_selection--;
  
  if(s_selection == -1) {
    window_stack_pop(true);
  }
  else
    layer_mark_dirty(s_canvas_layer);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_selection == 0 && s_withampm && s_digits[s_selection] == s_max[s_selection] - 1)
    s_digits[2] = !s_digits[2];
  
  s_digits[s_selection] += s_digits[s_selection] == s_max[s_selection] ? -s_max[s_selection] : 1;

  if(s_selection == 0 && s_withampm && s_digits[0] == 0)
    s_digits[0] = 1;
  
  temp_alarm.hour = s_digits[0];
  temp_alarm.minute = s_digits[1];
	
  layer_mark_dirty(s_canvas_layer);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_selection == 0 && s_withampm && s_digits[s_selection] == s_max[s_selection])
    s_digits[2] = !s_digits[2];
	  
  s_digits[s_selection] -= (s_digits[s_selection] == 0) ? -s_max[s_selection] : 1;
	
  if(s_selection == 0 && s_withampm && s_digits[0] == 0)
    s_digits[0] = s_max[0];
  
  temp_alarm.hour = s_digits[0];
  temp_alarm.minute = s_digits[1];
  
  layer_mark_dirty(s_canvas_layer);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 70, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 70, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void time_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // init hands
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, update_ui);
  layer_add_child(window_layer, s_canvas_layer);
  
  for(int i = 0; i < 3; i++) {
    s_input_layers[i] = text_layer_create(GRect((s_withampm?3:30) + i * (PIN_WINDOW_SPACING + PIN_WINDOW_SPACING) + OFFSET_LEFT, 60 + OFFSET_TOP, 40, 40));
#ifdef PBL_COLOR
    text_layer_set_text_color(s_input_layers[i], GColorWhite);
    text_layer_set_background_color(s_input_layers[i], GColorDarkGray);
#else
    text_layer_set_text_color(s_input_layers[i], GColorBlack);
    text_layer_set_background_color(s_input_layers[i], GColorWhite);
#endif
    text_layer_set_text(s_input_layers[i], "00");
    text_layer_set_font(s_input_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(s_input_layers[i], GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_input_layers[i]));
  }
  window_set_click_config_provider(window, click_config_provider);
  
  s_layer_countdown = text_layer_create(GRect(30, 60 + OFFSET_TOP+ 40 +25, 120, 40));
  text_layer_set_text_color(s_layer_countdown, GColorDarkGray);
  text_layer_set_background_color(s_layer_countdown, GColorWhite);
  text_layer_set_text(s_layer_countdown, "00");
  //text_layer_set_font(s_layer_countdown, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_layer_countdown, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_layer_countdown));
  
  s_time_status_layer = status_bar_layer_create();
  status_bar_layer_set_colors(s_time_status_layer, GColorWhite, GColorBlack);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_time_status_layer));
  
  layer_mark_dirty(s_canvas_layer);
}

static void time_window_unload(Window *window) {
  for(int i = 0; i < 3; i++) {
    text_layer_destroy(s_input_layers[i]);
  }
  layer_destroy(s_canvas_layer);
  //window_destroy(window);
}

static uint16_t menu_num_sections(struct MenuLayer* menu, void* callback_context) {
  if(alarm_ring_length()>0)
    return 2;
  return 1;
}

static uint16_t menu_num_rows(struct MenuLayer* menu, uint16_t section_index, void* callback_context) {
  if(section_index==0) {
    return 2;
  } else {
    return alarm_ring_length();
  }
  /*switch (section_index) {
    case MENU_SECTION_WEEKDAYS:
      return 8;
      break;
    case MENU_SECTION_OK:
      return 2;
    default:
      break;
  }
  return 7;*/
}

static int16_t menu_cell_height(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context) {
  return ITEM_HEIGHT;
}

static int16_t menu_header_height(struct MenuLayer *menu, uint16_t section_index, void *callback_context) {
/*#ifdef PBL_RECT
  return 16;
#else
  if(section_index == MENU_SECTION_WEEKDAYS)
  return 16;
  else return 0;
#endif*/
  if(section_index == 0) {
    return 0;
  }
  return 16;
}



/*static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* callback_context) {
  graphics_context_set_text_color(ctx, GColorWhite);
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorBlue);
  #else
    graphics_context_set_fill_color(ctx, GColorBlack);
  #endif
  GRect layer_size = layer_get_bounds(cell_layer);
  graphics_fill_rect(ctx,GRect(0,1,layer_size.size.w,14),0,GCornerNone);
  
#ifdef PBL_RECT
  graphics_draw_text(ctx, section_index==MENU_SECTION_OK?_("Update Alarm"):_("Weekdays"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(3, -2, layer_size.size.w - 3, 14), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
#else
  if (section_index==MENU_SECTION_WEEKDAYS)
  graphics_draw_text(ctx, _("Weekdays"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(3, -2, layer_size.size.w - 3, 14), GTextOverflowModeWordWrap,
                     GTextAlignmentCenter, NULL);
#endif
}*/

static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* callback_context) {
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorBlue);
  GRect layer_size = layer_get_bounds(cell_layer);
  graphics_fill_rect(ctx,GRect(0,1,layer_size.size.w,14),0,GCornerNone);
  
#ifdef PBL_RECT
  graphics_draw_text(ctx, section_index==MENU_SECTION_OK?_("Update Alarm"):_("Weekdays"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(3, -2, layer_size.size.w - 3, 14), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
#else
  if (section_index!=0)
  graphics_draw_text(ctx, _("Recent alarm times"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(3, -2, layer_size.size.w - 3, 14), GTextOverflowModeWordWrap,
                     GTextAlignmentCenter, NULL);
#endif
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context) {

/*  graphics_context_set_text_color(ctx, GColorBlack);*/
/*  graphics_context_set_fill_color(ctx, GColorBlack);*/

  char* text = NULL;
  GFont font = NULL;
  GRect layer_size = layer_get_bounds(cell_layer);
  
  if(cell_index->section == MENU_SECTION_OK)
  {
    if(cell_index->row==0) // OK
    {
      text = _("Set & Activate"),
      font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    }
    if(cell_index->row==1) // set text
    {
      text = _("Deactivate");
      font = fonts_get_system_font(FONT_KEY_GOTHIC_24);
    }
  } else {
    AlarmTime *alarm;
    alarm = alarm_ring_getTime(cell_index->row);
    snprintf(s_time_text, sizeof(s_time_text), "%02d:%02d",alarm->hour,alarm->minute);
    menu_cell_basic_draw(ctx, cell_layer, s_time_text, NULL, NULL);
  }
#ifdef PBL_RECT
  graphics_draw_text(ctx, text,
                     font,
                     GRect(3, -3 + OFFSET_ITEM_TOP, layer_size.size.w - 3, 28), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
#else
  graphics_draw_text(ctx, text,
                     font,
                     GRect(3, -3 + OFFSET_ITEM_TOP, layer_size.size.w - 3, 28), GTextOverflowModeWordWrap,
                     GTextAlignmentCenter, NULL);
#endif
}

// Alarm options window stuff
void window_load(Window* window)
{
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  s_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_num_sections,
    .get_num_rows = menu_num_rows,
    .get_cell_height = menu_cell_height,
    .get_header_height = menu_header_height,
    .draw_header = menu_draw_header,
    .draw_row = menu_draw_row,
    .select_click = menu_select,
  });
  
  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu, s_window);
  
#ifdef PBL_COLOR
  menu_layer_set_highlight_colors(s_menu,GColorCyan,GColorBlack);
  menu_layer_pad_bottom_enable(s_menu,false);
#endif
  // Add it to the window for display
  layer_add_child(window_layer, menu_layer_get_layer(s_menu));
  
  s_status_layer = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_layer, GColorWhite, GColorBlack);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_layer));
}

void window_unload(Window* window)
{
  menu_layer_destroy(s_menu);
  //gbitmap_destroy(check_icon);
}

static void menu_select(struct MenuLayer* menu, MenuIndex* cell_index, void* callback_context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu select"); 
  switch (cell_index->section) {
    case MENU_SECTION_OK:
        /*window_stack_pop(true);
        window_stack_pop(false);
        alarm_process();
        main_window_mark_dirty();*/
        if(cell_index->row==0) {
          APP_LOG(APP_LOG_LEVEL_DEBUG, "Push time setting window");
          window_stack_push(s_time_window, true);
        } else {
          temp_alarm.enabled = false;
          save_alarm(false);
          window_stack_pop(true);
        }
      break;
      case 1:
        temp_alarm.enabled = true;
        temp_alarm.hour = alarm_ring_getTime(cell_index->row)->hour;
        temp_alarm.minute = alarm_ring_getTime(cell_index->row)->minute;
        save_alarm(false);
        window_stack_pop(true);
      break;
    default:
      break;
  }
}