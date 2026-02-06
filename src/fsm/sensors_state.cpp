#include <Arduino.h>
#include "config/pins.h"
#include "sensors_state.h"

static LevelSensorsState levelState = LOW_LEVEL;
static LevelSensorsState pendingState = LOW_LEVEL;
static SensorState highSensorState = OFF;
static SensorState lowSensorState = OFF;
static unsigned long stateTimestamp = 0;
static const int DEBOUNCE_MS = 500;

LevelSensorsState calculateSensorsState(bool lowerWet, bool higherWet)
{
   if (!lowerWet && higherWet)
   {
      return ERROR;
   }
   if (!lowerWet && !higherWet)
   {
      return LOW_LEVEL;
   }
   if (lowerWet && !higherWet)
   {
      return MEDIUM_LEVEL;
   }
   return HIGH_LEVEL;
}

void update_sensors()
{
   int lowerWet = digitalRead(PIN_LOWER_SENSOR) == HIGH;
   int higherWet = digitalRead(PIN_HIGHER_SENSOR) == HIGH;

   const LevelSensorsState rawState = calculateSensorsState(lowerWet, higherWet);

   if (rawState != pendingState)
   {
      pendingState = rawState;
      stateTimestamp = millis();
      return;
   }
   if (millis() - stateTimestamp < DEBOUNCE_MS)
   {
      return;
   }
   if (levelState != pendingState)
   {
      levelState = pendingState;
   }
}

LevelSensorsState getSensorsState()
{
   return levelState;
}