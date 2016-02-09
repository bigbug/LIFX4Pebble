#pragma once
#include "alarm.h"

#define ALARMS_KEY 2
#define ALARMS_RING 3

void load_persistent_storage_alarms(Alarm *alarm);
void write_persistent_storage_alarms(Alarm *alarm);

void load_persistent_storage_alarmring(AlarmTimeRing *ring);
void write_persistent_storage_alarmring(AlarmTimeRing *ring);