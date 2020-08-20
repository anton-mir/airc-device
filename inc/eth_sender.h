
#ifndef AIRC_DEVICE_ETH_SENDER_H
#define AIRC_DEVICE_ETH_SENDER_H

#include "FreeRTOS.h"
#include "queue.h"

extern xQueueHandle QueueTransmitEthernet;


void eth_sender(void *pvParameters);

#endif //AIRC_DEVICE_ETH_SENDER_H
