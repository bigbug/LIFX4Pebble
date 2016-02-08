#include <pebble.h>
#include "alarm.h"
#include "comm.h"

/*time_t time_get_tomorrow(int hour, int minute) {
  //time_t timestamp = time(NULL);
  time_t timestamp = time_start_of_today();
  time_t wishdate = hour*60 + minute;
  
  if(timestamp<wishdate) {
    return clock_to_timestamp(TODAY, hour, minute);
  } else {
    //if()
  }
}

void alarm_set_wakeuptime() {
  //Check the event is not already scheduled
  if (!wakeup_query(s_wakeup_id, NULL)) {
    // Current time + 30 seconds
    time_t future_time = time(NULL) + 30;
  

    // Schedule wakeup event and keep the WakeupId
    s_wakeup_id = wakeup_schedule(future_time, WAKEUP_REASON, true);
    persist_write_int(PERSIST_KEY_WAKEUP_ID, s_wakeup_id);

    // Prepare for waking up later
    text_layer_set_text(s_output_layer, "This app will now wake up in 30 seconds.\n\nClose me!");
  }
}*/
Alarm myAlarm;

Alarm* alarm_get() {
  return &myAlarm;
}

void alarm_process() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Schedule wakeups!");
  if(!myAlarm.enabled) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "No enabled alarm. All wakeup events cancelled.");
    // turn off alarm
    wakeup_cancel_all();
  } else {
    time_t wakeup_time = alarm_get_time_of_wakeup(&myAlarm);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Wakeup scheduled for: %lu", (unsigned long)wakeup_time);
    if(myAlarm.secondsToLightUpBeforeAlarm > 0) {
      long timediff = wakeup_time - time(NULL) - myAlarm.secondsToLightUpBeforeAlarm;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Light up scheduled %d seconds before.", myAlarm.secondsToLightUpBeforeAlarm);
      if(timediff<0 || timediff<=60) {
        send(DURATION_ON, -timediff);
      } else {
        wakeup_schedule(timediff, WAKEUP_REASON_LIGHT_UP, true);
      }
    }
    myAlarm.alarm_id = wakeup_schedule(wakeup_time, WAKEUP_REASON_ALARM, true);
  }
}

void alarm_reset(Alarm *alarm)
{
  alarm->hour=0;
  alarm->minute=0;
  alarm->enabled=false;
  alarm->alarm_id=-1;
  alarm->secondsToLightUpBeforeAlarm = 120;
  alarm->flashingAfterXSnoozes = 1;
}

time_t clock_to_timestamp_precise(WeekDay day, int hour, int minute) {
  return (clock_to_timestamp(day, hour, minute)/60)*60;
}

time_t alarm_get_time_of_wakeup(Alarm *alarm)
{
  // Calculate time to wake up
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  time_t temp_timestamp;

  // Check if we may schedule the alarm today
  int current_weekday = t->tm_wday;
  temp_timestamp = clock_to_timestamp_precise((current_weekday+1)%7,alarm->hour,alarm->minute);
  if(temp_timestamp>(now + (60*60*24*7))) // more than one week away? This is today!
    temp_timestamp-=(60*60*24*7);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Diff to now: %d",(int)(temp_timestamp-now));
  return temp_timestamp;
}


void convert_24_to_12(int hour_in, int* hour_out, bool* am)
{
  *hour_out=hour_in%12;
  if (*hour_out==0) {
    *hour_out=12;
  }
  *am=hour_in<12;
}