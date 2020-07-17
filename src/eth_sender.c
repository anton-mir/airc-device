#include "eth_sender.h"
#include "eth_server.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/mem.h"
#include "lm335z.h"
#include "main.h"


void eth_sender(void *pvParameters){
    xData lReceivedValue;
    portBASE_TYPE xStatus;
    const portTickType xTicksToWait = 100 / portTICK_RATE_MS;
    for( ;; )
    {
            xStatus = xQueueReceive(xQueue, &lReceivedValue, xTicksToWait);
            if (xStatus == pdPASS) {
                sender_ethernet(lReceivedValue, sizeof(lReceivedValue));
            }
    }
}
