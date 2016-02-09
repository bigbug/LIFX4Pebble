#include <pebble.h>
#include "storage.h"

void load_persistent_storage_alarms(Alarm *alarm)
{
  if (persist_exists(ALARMS_KEY)) {
    persist_read_data(ALARMS_KEY,alarm,sizeof(Alarm));
  } else {
    alarm_reset(alarm);
  }
}

void write_persistent_storage_alarms(Alarm *alarm)
{
    persist_write_data(ALARMS_KEY,alarm,sizeof(Alarm));
}

void load_persistent_storage_alarmring(AlarmTimeRing *ring)
{
  if (persist_exists(ALARMS_KEY)) {
    persist_read_data(ALARMS_KEY,ring,sizeof(AlarmTimeRing));
  } else {
    alarm_ring_reset();
  }
}

void write_persistent_storage_alarmring(AlarmTimeRing *ring)
{
    persist_write_data(ALARMS_KEY,ring,sizeof(AlarmTimeRing));
}