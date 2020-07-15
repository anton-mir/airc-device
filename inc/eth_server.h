#ifndef ETH_SERVER_H
#define ETH_SERVER_H

#define SERVER_PORT                     (11333)
#define MAX_CLIENTS                     (4)

int sender_ethernet (void *ptr_buffer, int size);
void eth_server(void * const arg);


#endif /* ETH_SERVER_H */
