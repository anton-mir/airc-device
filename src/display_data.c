#include "FreeRTOS.h"
#include "queue.h"
#include "display_data.h"
#include "data_collector.h"
#include "data_structure.h"
#include "number_helper.h"
#include "eth_sender.h"
#include "wh1602.h"
#include "string.h"

dataPacket_S current_packet = {0};
UBaseType_t messages_in_queue = 0;

xQueueHandle displayQueueHandle = NULL;
typedef struct{
    uint8_t x;
    uint8_t y;
    uint8_t current_line;
}line_cntrl;

static line_cntrl line = {0};


static void change_line(){
    line.current_line++;
    if((line.current_line % 2) == 0){
        vTaskDelay(DISPLAY_TIME_PERIOD);
        lcd_clear();
        line.x = 0 ;
        line.y = 0;
        line.current_line = 0;
    }
    else {
        line.x = 0;
        line.y = 1;
    }
}

static void print_sensor_data(double data, char* sensor_name){
    lcd_print_string_at(sensor_name,line.x,line.y);
    //print value after name
    char sensor_data[20];
    line.x = strlen(sensor_name);
    ftoa(data,sensor_data,5);
    lcd_print_string_at(sensor_data,line.x,line.y);
    change_line();
}

void display_data_task(void *pvParams){
    displayQueueHandle = xQueueCreate(1,sizeof(dataPacket_S));
    for(;;)
    {
        messages_in_queue = uxQueueMessagesWaiting(displayQueueHandle);

        if (displayQueueHandle != NULL && messages_in_queue > 0)
        {
             xQueueReceive(displayQueueHandle,&current_packet,portMAX_DELAY);
        }
        else
        {
            print_sensor_data(current_packet.temp,"TEMP: " );
            print_sensor_data(current_packet.humidity,"HUMIDITY: ");
            print_sensor_data(current_packet.co2,"CO2: ");
            print_sensor_data(current_packet.tvoc,"TVOC: ");
            print_sensor_data(current_packet.pressure,"PRESSURE: ");
            print_sensor_data(current_packet.co,"CO: ");
            print_sensor_data(current_packet.co_temp,"CO_TEMP: ");
            print_sensor_data(current_packet.co_hum,"CO_HUM: ");
            print_sensor_data(current_packet.no2,"NO2: ");
            print_sensor_data(current_packet.no2_temp,"NO2_TEMP: ");
            print_sensor_data(current_packet.no2_hum,"NO2_HUM: ");
            print_sensor_data(current_packet.so2,"SO2: ");
            print_sensor_data(current_packet.so2_temp,"SO2_TEMP: ");
            print_sensor_data(current_packet.so2_hum,"SO2_HUM: ");
            print_sensor_data(current_packet.o3,"O3: ");
            print_sensor_data(current_packet.o3_temp,"O3_TEMP: ");
            print_sensor_data(current_packet.o3_hum,"O3_HUM: ");
            print_sensor_data(current_packet.hcho,"HCHO: ");
            print_sensor_data(current_packet.pm2_5,"PM2_5: ");
            print_sensor_data(current_packet.pm10,"PM10: ");
        }
    }
}
