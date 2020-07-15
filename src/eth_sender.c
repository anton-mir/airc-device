#include "eth_sender.h"
#include "eth_server.h"
#include "queue.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/mem.h"


static int clients[MAX_CLIENTS];

void eth_sender(void *pvParameters){
    long lReceivedValue;
    portBASE_TYPE xStatus;
    const portTickType xTicksToWait = 100 / portTICK_RATE_MS;
    char *Test="Test";
    for( ;; )
    {
        if( uxQueueMessagesWaiting( xQueue ) == 0 ) {
            xStatus = xQueueReceive(xQueue, &lReceivedValue, xTicksToWait);
            if (xStatus == pdPASS) {
                sender_ethernet(test,sizeof(test));

            }
        }
    }
    free(test);
}