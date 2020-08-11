#include "main.h"
#include "data_collector.h"
#include "data_structure.h"
#include "eth_sender.h"
#include "lm335z.h"
#include "i2c_ccs811sensor.h"


void data_collector(void *pvParameters)
{
    const portTickType xTicksToWait = 0;
    dataPacket_S packet={0,0,0,0,0,0,0,0,0,0,0,0};
    //TODO: Add waiting for other sensors.
    xEventGroupWaitBits(
            eg_task_started,
            (EG_INIT_STARTED | EG_ETHERIF_IN_STARTED | EG_LINK_STATE_STARTED |
            EG_DHCP_FSM_STARTED | ETH_SERVER_STARTED | ETH_SENDER_STARTED |
            EG_ANALOG_TEMP_STARTED | EG_I2C_CCS811_STARTED),
            pdFALSE,
            pdTRUE,
            portMAX_DELAY);
    xEventGroupSetBits(eg_task_started, EG_DATA_COLLECTOR_STARTED);
    for( ;; )
    {
	struct co2_tvoc co2_tvoc_t = get_co2_tvoc();
        packet.co2=co2_tvoc_t.co2;
        packet.tvoc=co2_tvoc_t.tvoc;

        //packet.co=;
        //packet.hcho=;
        //packet.humidity=;
        //packet.no2=;
        //packet.o3=;
        //packet.pm10=;
        //packet.pm2_5;
        //packet.pressure=;
        packet.temp=get_analog_temp();
        //packet.so2=;
        //TODO: Do return value processing.
        xQueueSendToBack(QueueTransmitEthernet, &packet, xTicksToWait);
        //xQueueSendToBack for wifi
        vTaskDelay(1000);
    }
}
