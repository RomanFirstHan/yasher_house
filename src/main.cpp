#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <UniversalTelegramBot.h>
#include "drivers/pumps/pump_station/pump_station_control.h"
#include "config/pins.h"
#include "telegram/telegram.h"
#include "fsm/sensors_state.h"

const bool statusLowerSensor = false;
const bool statusHigherSensor = false;
const bool isWiFiConnected = false;

// [WIFI] Блок – Взаимодействие с WiFi

// Константы для WiFi
const char *PASSWORD = "pobeda321";
const char *SSID = "POBEDA";
// const char *PASSWORD = "txRQShJK";
// const char *SSID = "RT-WiFi-7197";

unsigned long wifiTimeConnected = 0;
const int wifiDelay = 3000;
unsigned long wifiTimeAttempt = 0;
int countAttempt = 0;
wl_status_t currentWiFiStatus = WL_DISCONNECTED;

// Инициализация WiFi
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.println("Connecting to WiFi ..");
  currentWiFiStatus = WL_IDLE_STATUS;
  wifiTimeConnected = millis();
}

// Реконнект WiFi
const unsigned long timeoutsReconnectingWiFi[] = {
    10 * 1000,
    60 * 1000,
    3 * 60 * 1000,
    6 * 60 * 1000,
    8 * 60 * 1000,
    15 * 60 * 1000,
    60 * 60 * 1000,
    6 * 60 * 60 * 1000,
    12 * 60 * 60 * 1000};
const int numberOfAttempts = sizeof(timeoutsReconnectingWiFi) / sizeof(timeoutsReconnectingWiFi[0]);

void reconnectedWiFi()
{
  if (countAttempt >= numberOfAttempts)
  {
    currentWiFiStatus = WL_DISCONNECTED;
    return;
  }
  if (millis() - wifiTimeAttempt >= timeoutsReconnectingWiFi[countAttempt])
  {
    if (wifiTimeAttempt == 0)
      wifiTimeAttempt = millis();
    // Увеличивать счетчик попыток только если перешли в IDLE режим
    if (currentWiFiStatus == WL_IDLE_STATUS)
    {
      countAttempt++;
    }
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    currentWiFiStatus = WL_IDLE_STATUS;
  }
}

void checkWiFi()
{
  if (millis() - wifiTimeConnected <= wifiDelay)
    return;
  wifiTimeConnected = millis();
  Serial.print("WiFi.status(): ");
  Serial.println(WiFi.status());
  Serial.print("WiFi.SSID(): ");
  Serial.println(WiFi.SSID());
  if (WiFi.status() == WL_CONNECTED && currentWiFiStatus != WL_CONNECTED)
  {
    currentWiFiStatus = WL_CONNECTED;
    wifiTimeAttempt = 0;
    countAttempt = 0;
  }
  if (WiFi.status() != WL_CONNECTED && WiFi.status() != WL_IDLE_STATUS)
    reconnectedWiFi();
}

// [SETUP]
void setup()
{
  Serial.begin(115200); // Запуск Serial для отладки
  Serial.println("ESP32 Speaker Ready!");
  pinMode(PIN_LOWER_SENSOR, INPUT_PULLDOWN);
  pinMode(PIN_HIGHER_SENSOR, INPUT_PULLDOWN);
  pinMode(PIN_PUMP_STATION, OUTPUT);
  pinMode(PIN_PUMP_WELL, OUTPUT);
  pinMode(PIN_VALVE, OUTPUT);

  initTelegram();
  initWiFi();
}

// [LOOP]
void loop()
{
  update_sensors();
  pumpStationControl();
  checkWiFi();
  telegramCheckAndSend();
}