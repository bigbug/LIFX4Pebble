#include <pebble.h>
#include "storage.h"

void load_persistent_storage_alarms(Alarm *alarm)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Load alarm from persistent storage");
  if (persist_exists(ALARMS_KEY)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Alarm exists in persistent storage");
    persist_read_data(ALARMS_KEY,alarm,sizeof(Alarm));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Alarm loaded from persistent storage");
    if(alarm->enabled) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Alarm enabled");
    } else {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Alarm disabled");
    }
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
  if (persist_exists(ALARMS_RING)) {
    persist_read_data(ALARMS_RING,ring,sizeof(AlarmTimeRing));
  } else {
    alarm_ring_reset();
  }
}

void write_persistent_storage_alarmring(AlarmTimeRing *ring)
{
    persist_write_data(ALARMS_RING,ring,sizeof(AlarmTimeRing));
}


void load_persistent_storage_preferences(Preferences *pref)
{
  if (persist_exists(PREFERENCES_KEY)) {
    persist_read_data(PREFERENCES_KEY,pref,sizeof(Preferences));
  } else {
    preferences_reset();
  }
}
void write_persistent_storage_preferences(Preferences *pref)
{
  persist_write_data(PREFERENCES_KEY,pref,sizeof(Preferences));
}