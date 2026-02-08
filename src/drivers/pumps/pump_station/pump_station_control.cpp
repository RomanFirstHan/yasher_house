#include <Arduino.h>
#include "fsm/sensors_state.h"
#include "config/pins.h"
#include "telegram/telegram.h"

static LevelSensorsState currentState;
bool pendingWater = false;
bool isError = false;

void stopAllPumps()
{
   digitalWrite(PIN_PUMP_STATION, LOW);
   digitalWrite(PIN_PUMP_WELL, LOW);
}

void pumpStationControl()
{
   LevelSensorsState rawState = getSensorsState();

   if (currentState == rawState)
   {
      return;
   }

   if (rawState == ERROR && !isError)
   {
      isError = true;
      stopAllPumps();
      enqueueMessage(ADMIN_CHAT_ID, "ВНИМАНИЕ!!! ОШИБКА ДАТЧИКОВ, НАСОСЫ ОСТАНОВЛЕНЫ");
      return;
   }
   if (rawState == LOW_LEVEL)
   {
      pendingWater = true;
      digitalWrite(PIN_PUMP_STATION, LOW);
      digitalWrite(PIN_PUMP_WELL, HIGH);
      enqueueMessage(ADMIN_CHAT_ID, "Достигнут минимальный уровень: Насосная станция выключена, скважинный насос запущен");
   }
   if (rawState == MEDIUM_LEVEL && (currentState == HIGH_LEVEL || pendingWater == true))
   {
      digitalWrite(PIN_PUMP_STATION, HIGH);
      enqueueMessage(ADMIN_CHAT_ID, "Средний уровень воды: Скважинный насос включен, насосная станция вкллючена");
   }
   if (rawState == HIGH_LEVEL)
   {
      pendingWater = false;
      enqueueMessage(ADMIN_CHAT_ID, "Достигнут верхний вровень воды: Скважинный насос выключен");
      digitalWrite(PIN_PUMP_WELL, LOW);
   }
   if (rawState == MEDIUM_LEVEL && pendingWater == false)
   {
      enqueueMessage(ADMIN_CHAT_ID, "Средний уровень воды: Состояние насосов не изменено");
   }

   currentState = rawState;
}