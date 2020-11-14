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
#include "config_board.h"
#include <string.h>
#include "bmp280.h"

extern boxConfig_S device_config;
dataPacket_S dataPacets_buffer[DATA_PACKET_BUFFER_SIZE] = {0};
dataPacket_S packet={0};

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
        static long message_counter = 0;

        for (int8_t current_packet = 0; current_packet < DATA_PACKET_BUFFER_SIZE; current_packet++)
        {
            get_spec_sensors_data(&dataPacets_buffer[current_packet]);
            dataPacets_buffer[current_packet].temp   = get_temperature_bme280();
            dataPacets_buffer[current_packet].hcho   = get_HCHO();
            dataPacets_buffer[current_packet].pm10   = get_pm10();
            dataPacets_buffer[current_packet].pm2_5  = get_pm2_5();
            dataPacets_buffer[current_packet].humidity = get_humidity_bme280();
            dataPacets_buffer[current_packet].pressure = get_pressure_bme280();
            dataPacets_buffer[current_packet].tvoc = get_co2_tvoc().tvoc;
            dataPacets_buffer[current_packet].co2 = get_co2_tvoc().co2;

            vTaskDelay(1000);
        }

        avrg_data_packets(&dataPacets_buffer, DATA_PACKET_BUFFER_SIZE, &packet);

        ReadConfig(&device_config);

        packet.device_id = device_config.id;
        packet.device_message_counter = message_counter++;
        strncpy(packet.message_date_time, "Thu Aug 04 14:48:05 2016", 24); // TODO: get date time from ESP with AT+CIPSNTPTIME?
        packet.device_working_status = device_config.working_status;
        strncpy(packet.device_type, device_config.type, 19);
        strncpy(packet.device_description, device_config.description, 500);
        packet.latitude = device_config.latitude;
        packet.longitude = device_config.longitude;
        packet.altitude = device_config.altitude;

        // TODO: xQueueSendToBack for wifi
        // Send data to ethernet task
        xQueueSendToBack(QueueTransmitEthernet, &packet, portMAX_DELAY);
        // Send data to display task
        xQueueSendToBack(displayQueueHandle, &packet, portMAX_DELAY);

        // Circulate the air
        // fans_on();
        // vTaskDelay(FANS_WORKING_TIME);
        // fans_off();

        memset(dataPacets_buffer, 0, sizeof(dataPacets_buffer));
        memset(&packet, 0, sizeof(packet));
    }
}

void avrg_data_packets(dataPacket_S *buffer, int buf_size, dataPacket_S *result_packet){

    double no2_error, co_error, so2_error, o3_error = 0;

    for (int i = 0; i < buf_size; i++)
    {
        if (buffer[i].co < 0) // Has spec error code
        {
            result_packet->co = buffer[i].co;
            result_packet->co_hum = buffer[i].co_hum;
            result_packet->co_temp = buffer[i].co_temp;
            co_error = buffer[i].co;
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
            so2_error = buffer[i].so2;
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
            o3_error = buffer[i].o3;
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
            no2_error = buffer[i].no2;
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

    // NOTE: Might get negative SPEC values when have either error code or sensor is not calibrated
    if (co_error == 0)
    {
        result_packet->co /= buf_size;
        result_packet->co_hum /= buf_size;
        result_packet->co_temp /= buf_size;
    }
    else
    {
    result_packet->co = result_packet->co_hum = result_packet->co_temp = co_error;
    }

    if (so2_error == 0)
    {
        result_packet->so2 /= buf_size;
        result_packet->so2_hum /= buf_size;
        result_packet->so2_temp /= buf_size;
    }
    else
    {
        result_packet->so2 = result_packet->so2_hum = result_packet->so2_temp = so2_error;
    }

    if (o3_error == 0)
    {
        result_packet->o3 /= buf_size;
        result_packet->o3_hum /= buf_size;
        result_packet->o3_temp /= buf_size;
    }
    else
    {
        result_packet->o3 = result_packet->o3_hum = result_packet->o3_temp = o3_error;
    }

    if (no2_error == 0)
    {
        result_packet->no2 /= buf_size;
        result_packet->no2_hum /= buf_size;
        result_packet->no2_temp /= buf_size;
    }
    else
    {
        result_packet->no2 = result_packet->no2_hum = result_packet->no2_temp = no2_error;
    }

    if (result_packet->temp > 0) result_packet->temp /= buf_size;
    if (result_packet->hcho > 0) result_packet->hcho /= buf_size;
    if (result_packet->pm10 > 0) result_packet->pm10 /= buf_size;
    if (result_packet->pm2_5 > 0) result_packet->pm2_5 /= buf_size;
    if (result_packet->humidity > 0) result_packet->humidity /= buf_size;
    if (result_packet->tvoc > 0) result_packet->tvoc /= buf_size;
    if (result_packet->co2 > 0) result_packet->co2 /= buf_size;
}
