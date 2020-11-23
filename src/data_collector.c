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
double last_correct_co_value, last_correct_no2_value, last_correct_so2_value, last_correct_o3_value = 0;
double last_correct_co_hum_value, last_correct_no2_hum_value, last_correct_so2_hum_value, last_correct_o3_hum_value = 0;
double last_correct_co_temp_value, last_correct_no2_temp_value, last_correct_so2_temp_value, last_correct_o3_temp_value = 0;

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

            vTaskDelay(1000); // To ensure sensors data being updated
        }

        for (int8_t i = 0; i < DATA_PACKET_BUFFER_SIZE; i++)
        {
            if (dataPackets_buffer[i].co_err < 0) // Has spec error code
            {
                result_packet.co += (last_correct_co_value);
                result_packet.co_hum += (last_correct_co_hum_value);
                result_packet.co_temp += (last_correct_co_temp_value);
            }
            else
            {
                result_packet.co += (dataPackets_buffer[i].co);
                result_packet.co_hum += (dataPackets_buffer[i].co_hum);
                result_packet.co_temp += (dataPackets_buffer[i].co_temp);
                
                last_correct_co_value = (dataPackets_buffer[i].co);
                last_correct_co_hum_value = (dataPackets_buffer[i].co_hum);
                last_correct_co_temp_value = (dataPackets_buffer[i].co_temp);
            }

            if (dataPackets_buffer[i].so2_err < 0) // Has spec error code
            {
                result_packet.so2 += last_correct_so2_value;
                result_packet.so2_hum += last_correct_so2_hum_value;
                result_packet.so2_temp += last_correct_so2_temp_value;
            }
            else
            {
                result_packet.so2 += (dataPackets_buffer[i].so2);
                result_packet.so2_hum += (dataPackets_buffer[i].so2_hum);
                result_packet.so2_temp += (dataPackets_buffer[i].so2_temp);

                last_correct_so2_value = (dataPackets_buffer[i].so2);
                last_correct_so2_hum_value = (dataPackets_buffer[i].so2_hum);
                last_correct_so2_temp_value = (dataPackets_buffer[i].so2_temp);
            }

            if (dataPackets_buffer[i].o3_err < 0) // Has spec error code
            {
                result_packet.o3 += last_correct_o3_value;
                result_packet.o3_hum += last_correct_o3_hum_value;
                result_packet.o3_temp += last_correct_o3_temp_value;
            }
            else
            {
                result_packet.o3 += (dataPackets_buffer[i].o3);
                result_packet.o3_hum += (dataPackets_buffer[i].o3_hum);
                result_packet.o3_temp += (dataPackets_buffer[i].o3_temp);

                last_correct_o3_value = (dataPackets_buffer[i].o3);
                last_correct_o3_hum_value = (dataPackets_buffer[i].o3_hum);
                last_correct_o3_temp_value = (dataPackets_buffer[i].o3_temp);
            }

            if (dataPackets_buffer[i].no2_err < 0) // Has spec error code
            {
                result_packet.no2 += last_correct_no2_value;
                result_packet.no2_hum += last_correct_no2_hum_value;
                result_packet.no2_temp += last_correct_no2_temp_value;
            }
            else
            {
                result_packet.no2 += (dataPackets_buffer[i].no2);
                result_packet.no2_hum += (dataPackets_buffer[i].no2_hum);
                result_packet.no2_temp += (dataPackets_buffer[i].no2_temp);

                last_correct_no2_value = (dataPackets_buffer[i].no2);
                last_correct_no2_hum_value = (dataPackets_buffer[i].no2_hum);
                last_correct_no2_temp_value = (dataPackets_buffer[i].no2_temp);
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

        // NOTE: SPEC might send negative values when have either error code or sensor is not calibrated

        if (result_packet.co > 0) result_packet.co /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.co_hum > 0) result_packet.co_hum /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.co_temp > 0) result_packet.co_temp /= DATA_PACKET_BUFFER_SIZE;

        if (result_packet.so2 > 0) result_packet.so2 /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.so2_hum > 0) result_packet.so2_hum /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.so2_temp > 0) result_packet.so2_temp /= DATA_PACKET_BUFFER_SIZE;

        if (result_packet.o3 > 0) result_packet.o3 /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.o3_hum > 0) result_packet.o3_hum /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.o3_temp > 0) result_packet.o3_temp /= DATA_PACKET_BUFFER_SIZE;

        if (result_packet.no2 > 0) result_packet.no2 /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.no2_hum > 0) result_packet.no2_hum /= DATA_PACKET_BUFFER_SIZE;
        if (result_packet.no2_temp > 0) result_packet.no2_temp /= DATA_PACKET_BUFFER_SIZE;

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
        result_packet.device_message_counter = device_config.sent_packet_counter;
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

        device_config.sent_packet_counter = result_packet.device_message_counter + 1;
        WriteConfig(device_config);

        memset(&dataPackets_buffer, 0, sizeof(dataPacket_S)*DATA_PACKET_BUFFER_SIZE);
        memset(&result_packet, 0, sizeof(dataPacket_S));
    }
}
