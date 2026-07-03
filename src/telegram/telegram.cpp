#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <UniversalTelegramBot.h>
#include "config/pins.h"
#include "drivers/pumps/pump_station/pump_station_control.h"

// [TELEGRAM]

char ADMIN_CHAT_ID[32] = "396017793";

static const char *BOT_TOKEN = "8570607408:AAGrdnY5JCkopb1oYP6TjfXLYdBJiewS7Dg";
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

static bool initSendingMessage = false;
static bool sendMessageAfterStartWellPump = false;

static const unsigned int pollingBotDelay = 6000;
static unsigned long botLastTime;

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
   char chatId[32];
   String text;
   uint8_t retries;
};

constexpr uint8_t TG_QUEUE_SIZE = 10;
TgMessage tgQueue[TG_QUEUE_SIZE];
uint8_t queueHead = 0;
uint8_t queueTail = 0;
uint8_t queueCount = 0;
const uint8_t NUMBER_ATTEMPTS = 3;

void enqueueMessage(const char chat_id[32], const String &text)
{
   if (queueCount < TG_QUEUE_SIZE)
   {
      TgMessage &msg = tgQueue[queueTail];

      strncpy(msg.chatId, chat_id, sizeof(msg.chatId));
      msg.chatId[sizeof(msg.chatId) - 1] = '\0';
      tgQueue[queueTail].text = text;
      tgQueue[queueTail].retries = 0;
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

   if (!initSendingMessage)
   {
      bot.sendMessage(ADMIN_CHAT_ID, "ESP32 'Насосная станция' перезапустилась, в сети и приступила к работе");
      initSendingMessage = !initSendingMessage;
   };

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

void handleCommand(TelegramCommand cmd, char chatId[32])
{
   switch (cmd)
   {
   case CMD_STATUS:

      Serial.println("💡 LED включен!");
      break;

   case CMD_PUMP_ON:
      digitalWrite(PIN_PUMP_STATION, HIGH);
      enqueueMessage(chatId, messageWithTime("Включено"));
      Serial.println("🌑Ток на 13 порту!");
      break;
   case CMD_PUMP_OFF:
      digitalWrite(PIN_PUMP_STATION, LOW);
      enqueueMessage(chatId, messageWithTime("Выключено"));
      Serial.println("Ток на 13 порту выключен!");
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
      char chatId[32]; // выбираем размер, который точно влезет (Telegram chat_id обычно до 20 символов)
      strncpy(chatId, bot.messages[i].chat_id.c_str(), sizeof(chatId));
      chatId[sizeof(chatId) - 1] = '\0'; // безопасный null-терминатор
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
      if (pendingWater && !sendMessageAfterStartWellPump)
      {
         processQueue();
         sendMessageAfterStartWellPump = true;
         return;
      }
      if (sendMessageAfterStartWellPump && pendingWater)
      {
         return;
      }
      else if (sendMessageAfterStartWellPump)
      {
         sendMessageAfterStartWellPump = false;
      }

      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      if (numNewMessages > 0)
      {
         handleNewMessages(numNewMessages);
      }
      botLastTime = millis();
      processQueue();
   }
}

void initTelegram()
{
   client.setInsecure();
   bot.waitForResponse = 5000;
}
