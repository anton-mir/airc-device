#ifndef ETH_SERVER_H
#define ETH_SERVER_H

#define SERVER_PORT                     (11333)
#define MAX_CLIENTS                     (4)
#include "main.h"

int sender_ethernet (xData data, int size);
void eth_server(void * const arg);


#endif /* ETH_SERVER_H */
