#pragma once
#include <pebble.h>

#define WAKEUP_REASON_ALARM 0
#define WAKEUP_REASON_LIGHT_UP 1000

#define ALARM_TIME_LENGTH 5

//#define WAKEUP_REASON_SNOOZE

typedef struct Alarm{
  unsigned char hour;
  unsigned char minute;
  //bool weekdays_active[7];
  bool enabled;
  WakeupId alarm_id;
  int secondsToLightUpBeforeAlarm;
  int flashingAfterXSnoozes;
  //char description[DESCRIPTION_LENGTH+1];
}Alarm;

typedef struct AlarmTime{
  unsigned char hour;
  unsigned char minute;
  //char description[DESCRIPTION_LENGTH+1];
}AlarmTime;

typedef struct AlarmTimeRing {
  AlarmTime times[ALARM_TIME_LENGTH];
  unsigned char currentIndex;
  unsigned char length;
}AlarmTimeRing;

AlarmTimeRing* alarm_ring_get();
void alarm_ring_reset();
AlarmTime* alarm_ring_getTime(char index);
void alarm_ring_add(AlarmTime *time);
char alarm_ring_length();

Alarm* alarm_get();
void alarm_reset(Alarm *alarm);
void alarm_process();
time_t alarm_get_time_of_wakeup(Alarm *alarm);
void convert_24_to_12(int hour_in, int* hour_out, bool* am);