//
//  wakeup.c
//  alarms++
//
//  Created by Christian Reinbacher on 01.11.14.
//
//

#include "wakeup.h"
#include "storage.h"
#include "pwm_vibrate.h"
#include "comm.h"

static Window *s_main_window;
static TextLayer *s_output_layer;
//static TextLayer *s_description_layer;
static ActionBarLayer *action_bar;


static char output_text[10];
//static bool *s_snooze;
static int s_snoozes = 0;
//static Alarm *s_alarm;
static uint32_t s_segments[]={600,1};
static int s_vibe_counter = 0;
static int s_vibration_pattern = 0;
static VibePatternPWM s_pwmPat = {
  .durations = s_segments,
  .num_segments = 2
};

static GDrawCommandSequence *s_command_seq;
static int s_index = 0;
static Layer *s_canvas_layer;

static int s_vibration_duration = 0;
static int s_auto_snooze=false;
//static int s_last_z = 10000;

void do_vibrate(void);
void vibe_timer_callback(void* data);
AppTimer* vibe_timer = NULL;
AppTimer* cancel_vibe_timer = NULL;
bool cancel_vibrate=false;
//bool s_flip_to_snooze=false;

static void dismiss_click_handler(ClickRecognizerRef recognizer, void *context) {
  alarm_get()->enabled=false;
  window_stack_pop(true);
}

static void do_snooze(void)
{
  int snooze_delay = 1;
  //snooze_delay = load_persistent_storage_int(SNOOZE_KEY,10);
  time_t timestamp = time(NULL) + 60*snooze_delay;
  //alarm_get()->alarm_id = wakeup_schedule(timestamp,s_snoozes+1,true);
  wakeup_schedule(timestamp,s_snoozes+1,true);
  struct tm *t = localtime(&timestamp);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Scheduled snooze at %d.%d %d:%d",t->tm_mday, t->tm_mon+1,t->tm_hour,t->tm_min);
  window_stack_pop(true);
}

static void snooze_click_handler(ClickRecognizerRef recognizer, void *context) {
  do_snooze();
}

static void do_nothing_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_SELECT, do_nothing_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, do_nothing_click_handler);
  //bool longpress_dismiss = load_persistent_storage_bool(LONGPRESS_DISMISS_KEY,false);
  //bool topbutton_dismiss = load_persistent_storage_bool(TOP_BUTTON_DISMISS_KEY, true);
  /*if(longpress_dismiss)
    window_long_click_subscribe(topbutton_dismiss?BUTTON_ID_UP:BUTTON_ID_DOWN,1000,dismiss_click_handler,NULL);
  else*/
  window_single_click_subscribe(BUTTON_ID_UP, dismiss_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, snooze_click_handler);
}

uint32_t min(uint32_t val1,uint32_t val2) {
  return val1<val2?val1:val2;
}

void do_vibrate(void) {
  if(s_vibration_pattern) {
    s_pwmPat.durations[1] = (s_vibe_counter/s_vibration_pattern)+1;
    s_vibe_counter++;
    vibes_enqueue_custom_pwm_pattern(&s_pwmPat);
  }
  else
    vibes_long_pulse();
  
  if(s_snoozes>=preferences_get()->flashingAfterXSnoozes) {
    if(get_lfx_state_power()) {
      send(DURATION_OFF_MS, 50);
    } else {
      send(DURATION_ON_MS, 50);
    }
  }

  vibe_timer = app_timer_register(1000,vibe_timer_callback,NULL);
}

void vibe_timer_callback(void* data) {
  if(!cancel_vibrate)
    do_vibrate();
}

void cancel_vibe_timer_callback(void* data) {
  cancel_vibrate = true;
  if(s_auto_snooze)
    do_snooze();
}

static void update_text(struct tm *t) {
  //if(is_24h())
    snprintf(output_text, sizeof(output_text), "%02d:%02d",t->tm_hour,t->tm_min);
  /*else
  {
    int hour;
    bool is_am;
    convert_24_to_12(t->tm_hour, &hour, &is_am);
    snprintf(output_text, sizeof(output_text), "%02d:%02d %s",hour,t->tm_min,is_am?"AM":"PM");
  }*/
}

static void handle_tick(struct tm *t, TimeUnits units_changed) {
  update_text(t);
  layer_mark_dirty(text_layer_get_layer(s_output_layer));
}

static void next_frame_handler(void *context) {
  // Draw the next frame
  layer_mark_dirty(s_canvas_layer);

  // Continue the sequence
  app_timer_register(33, next_frame_handler, NULL);
}

static void update_proc(Layer *layer, GContext *ctx) {
  // Get the next frame
  GDrawCommandFrame *frame = gdraw_command_sequence_get_frame_by_index(s_command_seq, s_index);

  // If another frame was found, draw it    
  if (frame) {
    gdraw_command_frame_draw(ctx, s_command_seq, frame, GPoint(0, 23));
  }

  // Advance to the next frame, wrapping if neccessary
  int num_frames = gdraw_command_sequence_get_num_frames(s_command_seq);
  s_index++;
  if (s_index == num_frames) {
    s_index = 0;
  }
}

static void main_window_load(Window *window) {
  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  
  action_bar = action_bar_layer_create();
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider);
  //bool topbutton_dismiss = load_persistent_storage_bool(TOP_BUTTON_DISMISS_KEY, true);

  action_bar_layer_set_icon_animated(action_bar,BUTTON_ID_UP,gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CROSS_INV),true);
  action_bar_layer_set_icon_animated(action_bar,BUTTON_ID_DOWN,gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_ZZ_INV),true);

  action_bar_layer_add_to_window(action_bar,window);
  
  // Create output TextLayer
  
  s_output_layer = text_layer_create(GRect(0, bounds.size.h/2-(clock_is_24h_style()?0:4), bounds.size.w-ACTION_BAR_WIDTH, bounds.size.h));
  text_layer_set_text_alignment(s_output_layer, GTextAlignmentCenter);
  text_layer_set_font(s_output_layer,fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));

  //snprintf(output_text, sizeof(output_text), "00:00");
  text_layer_set_text(s_output_layer, output_text);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_output_layer));
  
  //snprintf(output_text, sizeof(output_text), "00:00");
  s_command_seq = gdraw_command_sequence_create_with_resource(RESOURCE_ID_CLOCK_SEQUENCE);
  // Create the canvas Layer
  s_canvas_layer = layer_create(GRect(PBL_IF_ROUND_ELSE(60,30)-ACTION_BAR_WIDTH/2, 0, bounds.size.w, bounds.size.h));
  // Set the LayerUpdateProc
  layer_set_update_proc(s_canvas_layer, update_proc);

  // Add to parent Window
  layer_add_child(window_get_root_layer(window), s_canvas_layer);
  app_timer_register(33, next_frame_handler, NULL);
  
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  update_text(t);
  
  s_vibration_pattern = 0;
  //s_vibration_pattern = load_persistent_storage_int(VIBRATION_PATTERN_KEY,0);
  //s_vibration_duration = load_persistent_storage_int(VIBRATION_DURATION_KEY, 2);

  s_vibration_duration = CONF_VIBRATION_DURATION;
  
  //s_auto_snooze = load_persistent_storage_bool(AUTO_SNOOZE_KEY, true);
  do_vibrate();
  // switch off vibration after x minutes
  /*switch (s_vibration_duration) {
    case 0:
    s_vibration_duration = 30;
    break;
    case 1:
    s_vibration_duration = 60;
    break;
    case 2:
    s_vibration_duration = 120;
    break;
    case 3:
    s_vibration_duration = 300;
    break;
    case 4:
    s_vibration_duration = 2;
    break;
    case 5:
    s_vibration_duration = 10;
    break;
    default:
    break;
  }*/
  cancel_vibe_timer = app_timer_register(1000*s_vibration_duration,cancel_vibe_timer_callback,NULL);
  
  // test snoozing with the accelerometer
  /*if(s_flip_to_snooze)
  {
    accel_tap_service_subscribe(&accel_tap_handler);
  }*/
}

static void main_window_unload(Window *window) {
  app_timer_cancel(vibe_timer);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Unload window");
  //if(s_flip_to_snooze)
  //  accel_tap_service_unsubscribe();
}

void wakeup_launch_window(int snoozes)
{
  s_snoozes = snoozes;
  
  // Create main Window
  s_main_window = window_create();

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  light_enable_interaction();
}