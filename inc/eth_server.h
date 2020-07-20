#ifndef ETH_SERVER_H
#define ETH_SERVER_H

#define SERVER_PORT                     (11333)
#define MAX_CLIENTS                     (4)

#define ETH_STATUS_SEND_OK              (0x0)
#define ETH_STATUS_1_CLIENT_ERROR       (0x1)
#define ETH_STATUS_2_CLIENT_ERROR       (0x2)
#define ETH_STATUS_3_CLIENT_ERROR       (0x4)
#define ETH_STATUS_4_CLIENT_ERROR       (0x8)
#include "main.h"

uint32_t sender_ethernet (void *data, uint32_t size);
void eth_server(void * const arg);


#endif /* ETH_SERVER_H */
