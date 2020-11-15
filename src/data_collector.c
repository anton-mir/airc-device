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
dataPacket_S result_packet={0};
dataPacket_S dataPackets_buffer[DATA_PACKET_BUFFER_SIZE] = {0};
int no2_error, co_error, so2_error, o3_error = 0;

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
            get_spec_sensors_data(&dataPackets_buffer[current_packet]);
            dataPackets_buffer[current_packet].temp_internal   = get_analog_temp();
            dataPackets_buffer[current_packet].temp   = get_temperature_bme280();
            dataPackets_buffer[current_packet].hcho   = get_HCHO();
            dataPackets_buffer[current_packet].pm10   = get_pm10();
            dataPackets_buffer[current_packet].pm2_5  = get_pm2_5();
            dataPackets_buffer[current_packet].humidity = get_humidity_bme280();
            dataPackets_buffer[current_packet].pressure = get_pressure_bme280();
            dataPackets_buffer[current_packet].tvoc = get_co2_tvoc().tvoc;
            dataPackets_buffer[current_packet].co2 = get_co2_tvoc().co2;

            vTaskDelay(1000);
        }

        for (int8_t i = 0; i < DATA_PACKET_BUFFER_SIZE; i++)
        {
            if (dataPackets_buffer[i].co < 0) // Has spec error code
            {
                result_packet.co = i == 0 ? 0 : dataPackets_buffer[i - 1].co; // Take previous correct value
                result_packet.co_hum = dataPackets_buffer[i].co_hum;
                result_packet.co_temp = dataPackets_buffer[i].co_temp;
                co_error = (int)dataPackets_buffer[i].co;
            }
            else
            {
                result_packet.co += (dataPackets_buffer[i].co);
                result_packet.co_hum += (dataPackets_buffer[i].co_hum);
                result_packet.co_temp += (dataPackets_buffer[i].co_temp);
            }

            if (dataPackets_buffer[i].so2 < 0) // Has spec error code
            {
                result_packet.so2 = i == 0 ? 0 : dataPackets_buffer[i - 1].so2; // Take previous correct value
                result_packet.so2_hum = dataPackets_buffer[i].so2_hum;
                result_packet.so2_temp = dataPackets_buffer[i].so2_temp;
                so2_error = (int)dataPackets_buffer[i].so2;
            }
            else
            {
                result_packet.so2 += (dataPackets_buffer[i].so2);
                result_packet.so2_hum += (dataPackets_buffer[i].so2_hum);
                result_packet.so2_temp += (dataPackets_buffer[i].so2_temp);
            }

            if (dataPackets_buffer[i].o3 < 0) // Has spec error code
            {
                result_packet.o3 = i == 0 ? 0 : dataPackets_buffer[i - 1].o3; // Take previous correct value
                result_packet.o3_hum = dataPackets_buffer[i].o3_hum;
                result_packet.o3_temp = dataPackets_buffer[i].o3_temp;
                o3_error = (int)dataPackets_buffer[i].o3;
            }
            else
            {
                result_packet.o3 += (dataPackets_buffer[i].o3);
                result_packet.o3_hum += (dataPackets_buffer[i].o3_hum);
                result_packet.o3_temp += (dataPackets_buffer[i].o3_temp);
            }

            if (dataPackets_buffer[i].no2 < 0) // Has spec error code
            {
                result_packet.no2 = i == 0 ? 0 : dataPackets_buffer[i - 1].no2; // Take previous correct value
                result_packet.no2_hum = dataPackets_buffer[i].no2_hum;
                result_packet.no2_temp = dataPackets_buffer[i].no2_temp;
                no2_error = (int)dataPackets_buffer[i].no2;
            }
            else
            {
                result_packet.no2 += (dataPackets_buffer[i].no2);
                result_packet.no2_hum += (dataPackets_buffer[i].no2_hum);
                result_packet.no2_temp += (dataPackets_buffer[i].no2_temp);
            }

            result_packet.temp_internal += (dataPackets_buffer[i].temp_internal);
            result_packet.temp     += (dataPackets_buffer[i].temp);
            result_packet.hcho     += (dataPackets_buffer[i].hcho);
            result_packet.pm10     += (dataPackets_buffer[i].pm10);
            result_packet.pm2_5    += (dataPackets_buffer[i].pm2_5);
            result_packet.humidity += (dataPackets_buffer[i].humidity);
            result_packet.pressure += (dataPackets_buffer[i].pressure);
            result_packet.tvoc     += (dataPackets_buffer[i].tvoc);
            result_packet.co2      += (dataPackets_buffer[i].co2);
        }

        // NOTE: Might get negative SPEC values when have either error code or sensor is not calibrated
        if (co_error == 0)
        {
            result_packet.co /= DATA_PACKET_BUFFER_SIZE;
            result_packet.co_hum /= DATA_PACKET_BUFFER_SIZE;
            result_packet.co_temp /= DATA_PACKET_BUFFER_SIZE;
        }
        else
        {
            result_packet.co = dataPackets_buffer[DATA_PACKET_BUFFER_SIZE-1].co; // Get last valid value
            result_packet.co_hum = result_packet.co_temp = (double)co_error; // Pass error code here
        }

        if (so2_error == 0)
        {
            result_packet.so2 /= DATA_PACKET_BUFFER_SIZE;
            result_packet.so2_hum /= DATA_PACKET_BUFFER_SIZE;
            result_packet.so2_temp /= DATA_PACKET_BUFFER_SIZE;
        }
        else
        {
            result_packet.so2 = dataPackets_buffer[DATA_PACKET_BUFFER_SIZE-1].so2; // Get last valid value
            result_packet.so2_hum = result_packet.so2_temp = (double)so2_error; // Pass error code here
        }

        if (o3_error == 0)
        {
            result_packet.o3 /= DATA_PACKET_BUFFER_SIZE;
            result_packet.o3_hum /= DATA_PACKET_BUFFER_SIZE;
            result_packet.o3_temp /= DATA_PACKET_BUFFER_SIZE;
        }
        else
        {
            result_packet.o3 = dataPackets_buffer[DATA_PACKET_BUFFER_SIZE-1].o3; // Get last valid value
            result_packet.o3_hum = result_packet.o3_temp = (double)o3_error; // Pass error code here
        }

        if (no2_error == 0)
        {
            result_packet.no2 /= DATA_PACKET_BUFFER_SIZE;
            result_packet.no2_hum /= DATA_PACKET_BUFFER_SIZE;
            result_packet.no2_temp /= DATA_PACKET_BUFFER_SIZE;
        }
        else
        {
            result_packet.no2 = dataPackets_buffer[DATA_PACKET_BUFFER_SIZE-1].no2; // Get last valid value
            result_packet.no2_hum = result_packet.no2_temp = (double)no2_error; // Pass error code here
        }

        if (result_packet.temp_internal > 0) result_packet.temp_internal /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.temp > 0) result_packet.temp /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.hcho > 0) result_packet.hcho /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.pm10 > 0) result_packet.pm10 /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.pm2_5 > 0) result_packet.pm2_5 /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.humidity > 0) result_packet.humidity /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.tvoc > 0) result_packet.tvoc /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.co2 > 0) result_packet.co2 /= DATA_PACKET_BUFFER_SIZE;

        ReadConfig(&device_config);

        result_packet.device_id = device_config.id;
        result_packet.device_message_counter = message_counter++;
        strncpy(result_packet.message_date_time, "Thu Aug 04 14:48:05 2016", 24); // TODO: get date time from ESP with AT+CIPSNTPTIME?
        result_packet.device_working_status = device_config.working_status;
        strncpy(result_packet.device_type, device_config.type, 19);
        strncpy(result_packet.device_description, device_config.description, 500);
        result_packet.latitude = device_config.latitude;
        result_packet.longitude = device_config.longitude;
        result_packet.altitude = device_config.altitude;

        // TODO: xQueueSendToBack for wifi
        // Send data to ethernet task
        xQueueSendToBack(QueueTransmitEthernet, &result_packet, portMAX_DELAY);
        // Send data to display task
        xQueueSendToBack(displayQueueHandle, &result_packet, portMAX_DELAY);

        // Circulate the air
        // fans_on();
        // vTaskDelay(FANS_WORKING_TIME);
        // fans_off();

        memset(&dataPackets_buffer, 0, sizeof(dataPacket_S)*DATA_PACKET_BUFFER_SIZE);
        memset(&result_packet, 0, sizeof(dataPacket_S));
        no2_error = co_error = so2_error = o3_error = 0;
    }
}
