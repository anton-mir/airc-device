#include "eth_sender.h"
#include "eth_server.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/mem.h"
#include "lm335z.h"
#include "main.h"

xQueueHandle QueueTransmitEthernet;


void eth_sender(void *pvParameters){
    xData lReceivedValue;
    portBASE_TYPE xStatus;
    xEventGroupWaitBits(
                eg_task_started,
                (EG_INIT_STARTED | EG_ETHERIF_IN_STARTED | EG_LINK_STATE_STARTED | EG_DHCP_FSM_STARTED | ETH_SERVER_STARTED ),
                pdFALSE,
                pdTRUE,
                portMAX_DELAY);
    QueueTransmitEthernet=xQueueCreate(1,sizeof(xData));
    xEventGroupSetBits(eg_task_started, ETH_SENDER_STARTED);
    for( ;; )
    {
            xStatus = xQueueReceive(QueueTransmitEthernet, &lReceivedValue, portMAX_DELAY);
            if (xStatus == pdPASS) {
                sender_ethernet(&lReceivedValue, sizeof(lReceivedValue));
            }
    }
}
