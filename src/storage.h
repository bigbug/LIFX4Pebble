#pragma once
#include "alarm.h"

#define ALARMS_KEY 1
#define ALARMS_RING 50

#define PREFERENCES_KEY 5

void load_persistent_storage_alarms(Alarm *alarm);
void write_persistent_storage_alarms(Alarm *alarm);

void load_persistent_storage_alarmring(AlarmTimeRing *ring);
void write_persistent_storage_alarmring(AlarmTimeRing *ring);

void load_persistent_storage_preferences(Preferences *pref);
void write_persistent_storage_preferences(Preferences *pref);