#include <stdio.h>
#include "main.h"
#include "data_collector.h"
#include "display_data.h"
#include "data_structure.h"
#include "eth_sender.h"
#include "lm335z.h"
#include "uart_sensors.h"
#include "fans.h"
#include "i2c_ccs811sensor.h"

#include "bmp280.h"

dataPacket_S dataPacets_buffer[DATA_PACKET_BUFFER_SIZE] = {0};

void data_collector(void *pvParameters)
{
    //TODO: Add waiting for other sensors.
    xEventGroupWaitBits(
            eg_task_started,
            (EG_INIT_STARTED | EG_ETHERIF_IN_STARTED | EG_LINK_STATE_STARTED |
            EG_DHCP_FSM_STARTED | EG_ETH_SERVER_STARTED | EG_ETH_SENDER_STARTED |
            EG_ANALOG_TEMP_STARTED | EG_UART_SENSORS_STARTED | EG_I2C_BME280_STARTED),
            pdFALSE,
            pdTRUE,
            portMAX_DELAY);
    xEventGroupSetBits(eg_task_started, EG_DATA_COLLECTOR_STARTED);
    //if something isn't working try change the stack size of  this task

    for( ;; )
    {
        static int current_packet = 0;

        if(current_packet == DATA_PACKET_BUFFER_SIZE)
        {
            dataPacket_S packet={0,0,0,0,0,0,0,0,0,0,0,0};
            current_packet = 0;
            avrg_data_packets(dataPacets_buffer,DATA_PACKET_BUFFER_SIZE,&packet);
            //send data to ethernet task
            xQueueSendToBack(QueueTransmitEthernet, &packet,portMAX_DELAY);
            //send data to display task
            xQueueSendToBack(displayQueueHandle,&packet,portMAX_DELAY);
            // fans_on();
            // vTaskDelay(FANS_WORKING_TIME);
            // fans_off();
        }
        get_spec_sensors_data(&dataPacets_buffer[current_packet]);
        dataPacets_buffer[current_packet].temp   = get_temperature_bme280(); 
        dataPacets_buffer[current_packet].hcho   = get_HCHO(); 
        dataPacets_buffer[current_packet].pm10   = get_pm10();
        dataPacets_buffer[current_packet].pm2_5  = get_pm2_5();
        dataPacets_buffer[current_packet].humidity = get_humidity_bme280();
        dataPacets_buffer[current_packet].pressure = get_pressure_bme280();
        dataPacets_buffer[current_packet].tvoc = get_co2_tvoc().tvoc;
        dataPacets_buffer[current_packet].co2 = get_co2_tvoc().co2;
        //TODO: Do return value processing.
        ++current_packet;
        //xQueueSendToBack for wifi
        vTaskDelay(1000);
    }
}

void avrg_data_packets(dataPacket_S *buffer, int buf_size, dataPacket_S *result_packet){
    
    for (int i = 0; i < buf_size; i++)
    {
        if (buffer[i].co < 0) // Has spec error code
        {
            result_packet->co = buffer[i].co;
            result_packet->co_hum = buffer[i].co_hum;
            result_packet->co_temp = buffer[i].co_temp;
        }
        else
        {
            result_packet->co += (buffer[i].co);
            result_packet->co_hum += (buffer[i].co_hum);
            result_packet->co_temp += (buffer[i].co_temp);
        }

        if (buffer[i].so2 < 0) // Has spec error code
        {
            result_packet->so2 = buffer[i].so2;
            result_packet->so2_hum = buffer[i].so2_hum;
            result_packet->so2_temp = buffer[i].so2_temp;
        }
        else
        {
            result_packet->so2 += (buffer[i].so2);
            result_packet->so2_hum += (buffer[i].so2_hum);
            result_packet->so2_temp += (buffer[i].so2_temp);
        }

        if (buffer[i].o3 < 0) // Has spec error code
        {
            result_packet->o3 = buffer[i].o3;
            result_packet->o3_hum = buffer[i].o3_hum;
            result_packet->o3_temp = buffer[i].o3_temp;
        }
        else
        {
            result_packet->o3 += (buffer[i].o3);
            result_packet->o3_hum += (buffer[i].o3_hum);
            result_packet->o3_temp += (buffer[i].o3_temp);
        }
        
        if (buffer[i].no2 < 0) // Has spec error code
        {
            result_packet->no2 = buffer[i].no2;
            result_packet->no2_hum = buffer[i].no2_hum;
            result_packet->no2_temp = buffer[i].no2_temp;
        }
        else
        {
            result_packet->no2 += (buffer[i].no2);
            result_packet->no2_hum += (buffer[i].no2_hum);
            result_packet->no2_temp += (buffer[i].no2_temp);
        }

        result_packet->temp     += (buffer[i].temp); 
        result_packet->hcho     += (buffer[i].hcho); 
        result_packet->pm10     += (buffer[i].pm10); 
        result_packet->pm2_5    += (buffer[i].pm2_5); 
        result_packet->humidity += (buffer[i].humidity); 
        result_packet->pressure += (buffer[i].pressure); 
        result_packet->tvoc     += (buffer[i].tvoc); 
        result_packet->co2      += (buffer[i].co2);
    }
    
    if (result_packet->co > 0) result_packet->co /= buf_size;
    if (result_packet->so2 > 0) result_packet->so2 /= buf_size;
    if (result_packet->o3 > 0) result_packet->o3 /= buf_size;
    if (result_packet->no2 > 0) result_packet->no2 /= buf_size;
    if (result_packet->temp > 0) result_packet->temp /= buf_size;
    if (result_packet->hcho > 0) result_packet->hcho /= buf_size;
    if (result_packet->pm10 > 0) result_packet->pm10 /= buf_size;
    if (result_packet->pm2_5 > 0) result_packet->pm2_5 /= buf_size;
    if (result_packet->humidity > 0) result_packet->humidity /= buf_size;
    if (result_packet->tvoc > 0) result_packet->tvoc /= buf_size;
    if (result_packet->co2 > 0) result_packet->co2 /= buf_size;
}
