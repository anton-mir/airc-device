#include<stdio.h>
#include "main.h"
#include "data_collector.h"
#include "data_structure.h"
#include "eth_sender.h"
#include "lm335z.h"
#include "uart_sensors.h"
#include "fans.h"



void data_collector(void *pvParameters)
{
    const portTickType xTicksToWait = 0;
    //TODO: Add waiting for other sensors.
    xEventGroupWaitBits(
            eg_task_started,
            (EG_INIT_STARTED | EG_ETHERIF_IN_STARTED | EG_LINK_STATE_STARTED |
            EG_DHCP_FSM_STARTED | EG_ETH_SERVER_STARTED | EG_ETH_SENDER_STARTED |
            EG_ANALOG_TEMP_STARTED | EG_UART_SENSORS_STARTED),
            pdFALSE,
            pdTRUE,
            portMAX_DELAY);
    xEventGroupSetBits(eg_task_started, EG_DATA_COLLECTOR_STARTED);
    //if something isn't working try change the stack size of  this task
    dataPacket_S dataPacets_buffer[DATA_PACKET_BUFFER_SIZE]; 
    for( ;; )
    {
        static int current_packet = 0;
        if(current_packet == DATA_PACKET_BUFFER_SIZE){
            dataPacket_S packet={0,0,0,0,0,0,0,0,0,0,0,0};
            current_packet = 0;
            avrg_data_packets(dataPacets_buffer,DATA_PACKET_BUFFER_SIZE,&packet);
            xQueueOverwrite(QueueTransmitEthernet, &packet);
            fans_on();
            vTaskDelay(FANS_WORKING_TIME);
            fans_off();
        }
        dataPacets_buffer[current_packet].co     = get_CO()->specPPB;
        dataPacets_buffer[current_packet].so2    = get_SO2()->specPPB;
        dataPacets_buffer[current_packet].no2    = get_NO2()->specPPB;
        dataPacets_buffer[current_packet].o3     = get_O3()->specPPB;
        dataPacets_buffer[current_packet].temp   = get_analog_temp(); 
        dataPacets_buffer[current_packet].hcho   = get_HCHO(); 
        dataPacets_buffer[current_packet].pm10   = get_pm10();
        dataPacets_buffer[current_packet].pm2_5  = get_pm2_5();
        //dataPacets_buffer[current_packet].humidity=;
        //dataPacets_buffer[current_packet].pressure=;
        //dataPacets_buffer[current_packet].tvoc=;
        //TODO: Do return value processing.
        ++current_packet;
        //xQueueSendToBack for wifi
        vTaskDelay(1000);
    }
}


void avrg_data_packets(dataPacket_S *buffer, int buf_size, dataPacket_S *result_packet){
    for(int i = 0; i < buf_size; i++){
        result_packet->co       += (buffer[i].co); 
        result_packet->so2      += (buffer[i].so2); 
        result_packet->o3       += (buffer[i].o3); 
        result_packet->temp     += (buffer[i].temp); 
        result_packet->hcho     += (buffer[i].hcho); 
        result_packet->pm10     += (buffer[i].pm10); 
        result_packet->pm2_5    += (buffer[i].pm2_5); 
        // result_packet->humidity += (buffer[i].humidity); 
        // result_packet->pressure += (buffer[i].pressure); 
        // result_packet->tvoc     += (buffer[i].tvoc); 
    }
        result_packet->co       /= buf_size; 
        result_packet->so2      /= buf_size;
        result_packet->o3       /= buf_size;
        result_packet->temp     /= buf_size; 
        result_packet->hcho     /= buf_size; 
        result_packet->pm10     /= buf_size; 
        result_packet->pm2_5    /= buf_size; 
        // result_packet->humidity /= buf_size; 
        // result_packet->pressure /= buf_size; 
        // result_packet->tvoc     /= buf_size; 
}
