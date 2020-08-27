#include "main.h"
#include "data_collector.h"
#include "data_structure.h"
#include "eth_sender.h"
#include "lm335z.h"
#include "uart_sensors.h"


void data_collector(void *pvParameters)
{
    const portTickType xTicksToWait = 0;
    dataPacket_S packet={0,0,0,0,0,0,0,0,0,0,0,0};
    //TODO: Add waiting for other sensors.
    xEventGroupWaitBits(
            eg_task_started,
            (EG_INIT_STARTED | EG_ETHERIF_IN_STARTED | EG_LINK_STATE_STARTED |
            EG_DHCP_FSM_STARTED | ETH_SERVER_STARTED | ETH_SENDER_STARTED |
            EG_ANALOG_TEMP_STARTED | EG_UART_SENSORS_STARTED),
            pdFALSE,
            pdTRUE,
            portMAX_DELAY);
    xEventGroupSetBits(eg_task_started, EG_DATA_COLLECTOR_STARTED);
    for( ;; )
    {
        packet.co=get_CO()->specPPB;
        packet.so2=get_SO2()->specPPB;
        packet.no2=get_NO2()->specPPB;
        packet.o3=get_O3()->specPPB;
        packet.temp=get_analog_temp();
        //packet.hcho=;
        //packet.humidity=;
        //packet.pm10=;
        //packet.pm2_5;
        //packet.pressure=;
        //packet.tvoc=;
        //TODO: Do return value processing.
        xQueueSendToBack(QueueTransmitEthernet, &packet, xTicksToWait);
        //xQueueSendToBack for wifi
        vTaskDelay(1000);
    }
}