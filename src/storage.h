#pragma once
#include "alarm.h"

#define ALARMS_KEY 2

void load_persistent_storage_alarms(Alarm *alarm);
void write_persistent_storage_alarms(Alarm *alarm);