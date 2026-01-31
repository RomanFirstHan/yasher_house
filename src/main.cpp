#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <UniversalTelegramBot.h>

#define PIN_LOWER_SENSOR 32
#define PIN_HIGHER_SENSOR 14
#define PIN_PUMP_STATION 13
#define PIN_PUMP_WELL 19
#define PIN_PUMP_WELL 19
#define PIN_VALVE 19
#define CHAT_ID 396017793

const bool statusLowerSensor = false;
const bool statusHigherSensor = false;
const bool isWiFiConnected = false;

// [WIFI] Блок – Взаимодействие с WiFi

// Константы для WiFi
const char *PASSWORD = "txRQShJK";
const char *SSID = "RT-WiFi-7197";

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
      Serial.println("Count++");
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

// [TELEGRAM]

const char *BOT_TOKEN = "8570607408:AAGrdnY5JCkopb1oYP6TjfXLYdBJiewS7Dg";
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

bool firstSendMessage = false;

const unsigned int pollingBotDelay = 6000;
unsigned long botLastTime;

enum TelegramCommand
{
  CMD_STATUS,
  CMD_DEAD_MAN
};

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    String chatId = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String fromName = bot.messages[i].from_name;
    Serial.print("chatId");
    Serial.println(chatId);
  }
}

void telegramCheckAndSend()
{
  if (WiFi.status() == WL_CONNECTED && millis() - botLastTime > pollingBotDelay)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    handleNewMessages(numNewMessages);
    botLastTime = millis();
  }
}

// [SETUP]
void setup()
{
  Serial.begin(115200); // Запуск Serial для отладки
  Serial.println("ESP32 Speaker Ready!");
  client.setInsecure();

  pinMode(PIN_LOWER_SENSOR, INPUT);
  pinMode(PIN_HIGHER_SENSOR, INPUT);
  pinMode(PIN_PUMP_STATION, OUTPUT);
  tone(PIN_PUMP_STATION, 420);
  pinMode(PIN_PUMP_WELL, OUTPUT);
  pinMode(PIN_VALVE, OUTPUT);

  initWiFi();
}

// [LOOP]
void loop()
{
  checkWiFi();
  telegramCheckAndSend();
}