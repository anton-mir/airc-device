#include "eth_sender.h"
#include "eth_server.h"
#include "main.h"
#include "data_structure.h"

xQueueHandle QueueTransmitEthernet;


void eth_sender(void *pvParameters){
    dataPacket_S lReceivedValue;
    portBASE_TYPE xStatus;
    xEventGroupWaitBits(
                eg_task_started,
                (EG_INIT_STARTED | EG_ETHERIF_IN_STARTED | EG_LINK_STATE_STARTED |
                EG_DHCP_FSM_STARTED | EG_ETH_SERVER_STARTED ),
                pdFALSE,
                pdTRUE,
                portMAX_DELAY);
    QueueTransmitEthernet=xQueueCreate(1,sizeof(dataPacket_S));
    xEventGroupSetBits(eg_task_started, EG_ETH_SENDER_STARTED);
    for( ;; )
    {
            xStatus = xQueueReceive(QueueTransmitEthernet, &lReceivedValue, portMAX_DELAY);
            if (xStatus == pdPASS)
            {
                sender_ethernet(&lReceivedValue, sizeof(dataPacket_S));
            }
    }
}
