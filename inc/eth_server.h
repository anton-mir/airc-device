#ifndef ETH_SERVER_H
#define ETH_SERVER_H

#define SERVER_PORT                     (11333)
#define MAX_CLIENTS                     (4)

void eth_server(void * const arg);
int get_eth_clients(int *clients_buf, uint32_t buf_len);

#endif /* ETH_SERVER_H */
