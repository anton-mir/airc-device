#include "main.h"
#include "data_collector.h"
#include "data_structure.h"
#include "eth_sender.h"
#include "lm335z.h"


void data_collector(void *pvParameters)
{
    const portTickType xTicksToWait = 100 / portTICK_RATE_MS;
    dataPacket_S packet={0,0,0,0,0,0,0,0,0,0,0,0};
    //Add waiting for other sensors
    xEventGroupWaitBits(
            eg_task_started,
            (EG_INIT_STARTED | EG_ETHERIF_IN_STARTED | EG_LINK_STATE_STARTED |
            EG_DHCP_FSM_STARTED | ETH_SERVER_STARTED | ETH_SENDER_STARTED |
            EG_ANALOG_TEMP_STARTED),
            pdFALSE,
            pdTRUE,
            portMAX_DELAY);
    xEventGroupSetBits(eg_task_started, EG_DATA_COLLECTOR_STARTED);
    for( ;; )
    {
        //packet.co=;
        //packet.co2=;
        //packet.hcho=;
        //packet.humidity=;
        //packet.no2=;
        //packet.o3=;
        //packet.pm10=;
        //packet.pm2_5;
        //packet.pressure=;
        packet.temp=get_analog_temp();
        //packet.tvoc=;
        //packet.so2=;
        xQueueSendToBack(QueueTransmitEthernet, &packet, xTicksToWait);
        //xQueueSendToBack for wifi
        vTaskDelay(1000);
    }
}