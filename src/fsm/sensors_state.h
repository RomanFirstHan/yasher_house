#pragma once
#include <Arduino.h>

enum LevelSensorsState
{
   HIGH_LEVEL,
   MEDIUM_LEVEL,
   LOW_LEVEL,
   ERROR,
};

void init_sensors();
void update_sensors();
LevelSensorsState getSensorsState();
