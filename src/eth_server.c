#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "main.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/mem.h"
#include "eth_server.h"

#define CHECK_CONNECTION_BUF_SIZE       (32)

static int clients[MAX_CLIENTS];
static uint8_t check_conn_buf[CHECK_CONNECTION_BUF_SIZE];
SemaphoreHandle_t clients_mut;

int sender_ethernet (void *ptr_buffer, int size){
    int err=0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0) {
            err=lwip_write(clients[i], ptr_buffer, size);
            if(err==-1){
                //For error
            }

        }
    }
}

static int add_client(int new_clientfd)
{
    int retval = -EINVAL;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == 0) {
            xSemaphoreTake(clients_mut, portMAX_DELAY);
            clients[i] = new_clientfd;
            xSemaphoreGive(clients_mut);

            retval = 0;
            break;
        }
    }


    return retval;
}

void eth_server(void * const arg)
{
    int listenfd;
    fd_set readfds;
    int maxfd;
    int clientfd;
    int err;
    int read_len;
    struct sockaddr_in server_addr;
    struct sockaddr_storage client_addr;
    socklen_t client_len;
    struct netif *netif = (struct netif *)arg;

    memset(&server_addr, 0, sizeof(server_addr));

    clients_mut = xSemaphoreCreateMutex();
    if (!clients_mut) {
        LWIP_ASSERT("server(): cannot create mutex", 0);
        return;
    }
    /* Notify init task that server task has been started */
    xEventGroupSetBits(eg_task_started, ETH_SERVER_STARTED);

    listenfd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    LWIP_ASSERT("server(): socket create failed", listenfd >= 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = PP_HTONL(INADDR_ANY);
    server_addr.sin_port = lwip_htons(SERVER_PORT);

    err = lwip_bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err < 0) {
        LWIP_ASSERT("server(): socket bind failed", 0);
    }

    err = lwip_listen(listenfd, MAX_CLIENTS);
    if (err < 0) {
        LWIP_ASSERT("server(): socket listen failed", 0);
    }

    for (;;) {
        FD_ZERO(&readfds);
        FD_SET(listenfd, &readfds);
        maxfd = listenfd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            clientfd = clients[i];

            if (clientfd > 0) {
                FD_SET(clientfd, &readfds);
            }

            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
        }

        err = select((maxfd + 1), &readfds, NULL, NULL, NULL);
        if (err < 0)
            continue;

        /* check for new client */
        if (FD_ISSET(listenfd, &readfds)) {
            clientfd = lwip_accept(
                    listenfd,
                    (struct sockaddr *)&client_addr,
                    &client_len);
            if (clientfd < 0)
                continue;

            err = add_client(clientfd);
            /* if err < 0 - no free slots for new connection
             * client is rejected */
        }

        /* check for already connected clients */
        for (int i = 0; i < MAX_CLIENTS; i++) {
            clientfd = clients[i];

            if (clientfd > 0 && FD_ISSET(clientfd, &readfds)) {
                read_len = lwip_read(
                        clientfd,
                        check_conn_buf,
                        CHECK_CONNECTION_BUF_SIZE);

                if (read_len <= 0) {
                    /* client disconnected or error - drop this connection */
                    lwip_close(clientfd);

                    xSemaphoreTake(clients_mut, portMAX_DELAY);
                    clients[i] = 0;
                    xSemaphoreGive(clients_mut);
                }
            }
        }
    }
}
