#include "FreeRTOS.h"
#include "queue.h"
#include "display_data.h"
#include "data_collector.h"
#include "data_structure.h"
#include "number_helper.h"
#include "eth_sender.h"
#include "wh1602.h"
#include "string.h"


xQueueHandle displayQueueHandle = NULL;
enum sensor_indexes
{
    TEMP = 0,
    HUMIDITY,
    CO2,
    TVOC,
    PRESSURE,
    CO,
    NO2,
    SO2,
    O3,
    HCHO,
    PM2_5,
    PM10
};

void display_data_task(void *pvParams){
    char *sensors_names[] = 
    {
      "TEMP: ",  
      "HUMIDITY: ",
      "CO2: ",
      "TVOC: ",
      "PRESSURE: ",
      "CO: ",
      "NO2: ",
      "SO2: ",
      "O3: ",
      "HCHO: ",
      "PM2_5: ",
      "PM10: "
    };
    displayQueueHandle = xQueueCreate(1,sizeof(dataPacket_S));
    
    size_t sensors_names_size = sizeof(sensors_names)/sizeof(sensors_names[0]);
    for(;;){
        dataPacket_S current_packet = {0,0,0,0,0,0,0,0,0,0,0,0};
        //pointer to the first field of the struct
        double *p_currnet_packet = (double*)&current_packet; 
        /*
            reads an item from ethernet queue, 
            but without removing the item from the queue
        */
        if((displayQueueHandle != NULL) &&
           (xQueueReceive(displayQueueHandle,&current_packet,portMAX_DELAY) == pdTRUE))
         {
            uint8_t x = 0;
            uint8_t y = 0;
            /*
                **************************
                *SENSOR_NAME: VALUE      *
                *SENSOR_NAME: VALUE      *
                **************************
            */
            for(int i = 0; i < sensors_names_size; i++){
                if(i != 0 && (i%2) == 0){
                    vTaskDelay(DISPLAY_TIME_PERIOD);
                    lcd_clear();
                    x = 0 ;
                    y = 0;
                }
                else if((i%2) != 0){
                    x = 0;
                    y = 1;
                }
                //print name
                lcd_print_string_at(sensors_names[i],x,y);
                //print value after name
                char sensor_data[20];
                x = strlen(sensors_names[i]);
                double var_value = (double)*(p_currnet_packet+i);
                ftoa(var_value,sensor_data,5);
                lcd_print_string_at(sensor_data,x,y);
            }
        }

    }
}