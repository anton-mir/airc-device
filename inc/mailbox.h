#ifndef MAILBOX_H
#define MAILBOX_H

#include "queue.h"
#include "dataGetter.h"
QueueHandle_t mailBoxCreate();
 void putToMailBox(QueueHandle_t mailBoxID, dataPacket_S *data);
 void receiveFromMailBox(QueueHandle_t mailBoxID, dataPacket_S* buffer);
 
 #endif

