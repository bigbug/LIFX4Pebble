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