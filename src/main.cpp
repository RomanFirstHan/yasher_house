#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <UniversalTelegramBot.h>
#include "drivers/pumps/pump_station/pump_station_control.h"
#include "config/pins.h"

#define ADMIN_CHAT_ID "396017793"

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

// [TELEGRAM]

const char *BOT_TOKEN = "8570607408:AAGrdnY5JCkopb1oYP6TjfXLYdBJiewS7Dg";
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

bool initSendingMessage = false;

const unsigned int pollingBotDelay = 6000;
unsigned long botLastTime;

enum TelegramCommand
{
  CMD_STATUS,
  CMD_PUMP_ON,
  CMD_PUMP_OFF,
  CMD_UNKNOWN
};

// Telegram очередь сообщений
struct TgMessage
{
  String chatId;
  String text;
  uint8_t retries;
};

constexpr uint8_t TG_QUEUE_SIZE = 6;
TgMessage tgQueue[TG_QUEUE_SIZE];
uint8_t queueHead = 0;
uint8_t queueTail = 0;
uint8_t queueCount = 0;
const uint8_t NUMBER_ATTEMPTS = 3;

void enqueueMessage(const String &chat_id, const String &text)
{
  if (queueCount < TG_QUEUE_SIZE)
  {
    tgQueue[queueTail] = {chat_id, text, 0};
    queueTail = (queueTail + 1) % TG_QUEUE_SIZE;
    queueCount++;
  }
  else
  {
    Serial.println("Очередь сообщений переполнена!");
  }
}

void processQueue()
{
  if (queueCount == 0)
    return;

  TgMessage &msg = tgQueue[queueHead];
  if (bot.sendMessage(msg.chatId, msg.text))
  {
    queueHead = (queueHead + 1) % TG_QUEUE_SIZE;
    queueCount--;
    Serial.println("Сообщение доставлено");
  }
  else
  {
    if (++msg.retries > NUMBER_ATTEMPTS)
    {
      queueHead = (queueHead + 1) % TG_QUEUE_SIZE;
      queueCount--;
      Serial.println("Сообщение не доставлено и удалено после n попыток");
    }
  }
}

// Парсинг телеграм команд для switch/case
TelegramCommand parseCommand(const String &cmd)
{
  if (cmd == "status" || cmd == "/status")
    return CMD_STATUS;
  if (cmd == "pumpon" || cmd == "/pumpon")
    return CMD_PUMP_ON;
  if (cmd == "pumpoff" || cmd == "/pumpoff")
    return CMD_PUMP_OFF;
  return CMD_UNKNOWN;
}

String messageWithTime(String mes)
{
  return String(millis()) + ": " + mes;
}

void handleCommand(TelegramCommand cmd, const String &chatId)
{
  switch (cmd)
  {
  case CMD_STATUS:

    Serial.println("💡 LED включен!");
    break;

  case CMD_PUMP_ON:
    digitalWrite(PIN_PUMP_STATION, HIGH);
    enqueueMessage(chatId, messageWithTime("Включено"));
    Serial.println("🌑 LED выключен!");
    break;
  case CMD_PUMP_OFF:
    digitalWrite(PIN_PUMP_STATION, LOW);
    enqueueMessage(chatId, messageWithTime("Выключено"));
    Serial.println("🌑 LED выключен!");
    break;
  case CMD_UNKNOWN:
  default:
    Serial.println("❌ Неизвестная команда!");
    break;
  }
}

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    Serial.print("Кол-во сообщений: ");
    Serial.println(numNewMessages);
    String chatId = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String fromName = bot.messages[i].from_name;
    const TelegramCommand cmd = parseCommand(text);
    handleCommand(cmd, chatId);
    Serial.print("chatId");
    Serial.println(chatId);
  }
}

void telegramCheckAndSend()
{
  if (WiFi.status() == WL_CONNECTED && millis() - botLastTime > pollingBotDelay)
  {
    if (!initSendingMessage)
    {
      enqueueMessage(ADMIN_CHAT_ID, "ESP32 'Насосная станция' перезапустилась, в сети и приступила к работе");
      initSendingMessage = !initSendingMessage;
    };
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages > 0)
    {
      handleNewMessages(numNewMessages);
    }
    botLastTime = millis();
    processQueue();
  }
}

// [SETUP]
void setup()
{
  Serial.begin(115200); // Запуск Serial для отладки
  Serial.println("ESP32 Speaker Ready!");
  client.setInsecure();

  bot.waitForResponse = 4000;
  pinMode(PIN_LOWER_SENSOR, INPUT);
  pinMode(PIN_HIGHER_SENSOR, INPUT);
  pinMode(PIN_PUMP_STATION, OUTPUT);
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