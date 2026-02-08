#pragma once

#include <Arduino.h>

extern char ADMIN_CHAT_ID[32];

void enqueueMessage(const char chat_id[32], const String &text);
void telegramCheckAndSend();
void initTelegram();