#pragma once
#include <Arduino.h>

enum LevelSensorsState
{
   HIGH_LEVEL,
   MEDIUM_LEVEL,
   LOW_LEVEL,
   ERROR,
};

void update_sensors();
LevelSensorsState getSensorsState();
