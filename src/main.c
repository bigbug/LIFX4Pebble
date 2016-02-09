#include <pebble.h>
#include "mainMenu.h"
#include "comm.h"
#include "progress_layer_window.h"
#include "wakeup.h"
#include "storage.h"

static bool phoneConnected = false;

static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Fail reason: %d", (int)reason);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received message");
  
  // Get the first pair
  Tuple *data = dict_find(iterator, LIFX_STATE_POWER);
  if (data) {
    set_lfx_state_power(data->value->uint8);
    //lfx_state_power = data->value->uint8;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Power state: %d", get_lfx_state_power());
  }
  
  if(!phoneConnected) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Phone was not Connected");
    progress_layer_window_pop();
    send(REQUEST_STATE, 1);
    phoneConnected = true;
  }
  
  main_window_mark_dirty();
}

static void wakeup_handler(WakeupId id, int32_t reason) {
  // The app has woken!
  
  if(reason == WAKEUP_REASON_LIGHT_UP) {
    send(DURATION_ON, alarm_get()->secondsToLightUpBeforeAlarm);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App was woken up!");
    wakeup_launch_window(reason);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

bool perform_wakeup_tasks()
{ 
  // Was this a wakeup?
  if(launch_reason() == APP_LAUNCH_WAKEUP) {
    // The app was started by a wakeup
    WakeupId id = 0;
    int32_t reason = 0;
    
    // Get details and handle the wakeup
    wakeup_get_launch_event(&id, &reason);
    
    // Get the current time
    //time_t now = time(NULL);
    //struct tm *t = localtime(&now);
    //update_text(t);
    
    //handle_tick(t, MINUTE_UNIT);
    
    wakeup_launch_window(reason);
    return true;
    //wakeup_handler(id, (int32_t)alarms);
  }
  return false;
}

static void init(void) {
  load_persistent_storage_alarms(alarm_get());
  load_persistent_storage_alarmring(alarm_ring_get());
  
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  
  // Subscribe to Wakeup API
  //wakeup_service_subscribe(wakeup_handler);
    // Subscribe to Wakeup API
  wakeup_service_subscribe(wakeup_handler);
  
  wakeup_cancel_all();
  //alarm_reset(alarm_get());
  alarm_process();

  // Open AppMessage
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  if(!perform_wakeup_tasks()) {
    main_window_init();
    //progress_layer_window_push();
  }
  
  // Create new dictation session
  //s_dictation_session = dictation_session_create(sizeof(s_last_text), dictation_session_callback, NULL);
}



static void deinit(void) {
  write_persistent_storage_alarms(alarm_get());
  write_persistent_storage_alarmring(alarm_ring_get());
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}