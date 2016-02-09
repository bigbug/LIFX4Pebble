#include <pebble.h>
#include "alarm.h"
#include "comm.h"

Alarm myAlarm;
AlarmTimeRing myRing;

Alarm* alarm_get() {
  return &myAlarm;
}

void alarm_ring_reset() {
  myRing.currentIndex = 0;
  myRing.length = 0;
}

AlarmTimeRing* alarm_ring_get() {
  return &myRing;
}

AlarmTime* alarm_ring_getTime(char index) {
  int ind = myRing.currentIndex - 1 - index;
  if(ind<0) {
    ind += ALARM_TIME_LENGTH;
  }
  return &(myRing.times[ind]);
}

void alarm_ring_add(AlarmTime *time) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Adding alarm with hour: %d and minute: %d", time->hour, time->minute);
  APP_LOG(APP_LOG_LEVEL_WARNING, "Ring current index: %d", myRing.currentIndex);
  myRing.times[myRing.currentIndex].hour   = time->hour;
  myRing.times[myRing.currentIndex].minute = time->minute;
  
  myRing.currentIndex = (myRing.currentIndex + 1) % (ALARM_TIME_LENGTH+1);
  myRing.length = myRing.length+1;
  if(myRing.length > ALARM_TIME_LENGTH)
    myRing.length = ALARM_TIME_LENGTH;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Ring stats. currentIndex: %d , current length: %d", myRing.currentIndex, myRing.length);
}

char alarm_ring_length() {
  return myRing.length;
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
  /*if(temp_timestamp>(now + (60*60*24*7))) // more than one week away? This is today!
    temp_timestamp-=(60*60*24*7);*/
  // subtract til its less than a day away
  while(temp_timestamp-now>60*60*24)
    temp_timestamp -= 60*60*24;
  
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