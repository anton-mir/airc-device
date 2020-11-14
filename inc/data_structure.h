#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#define DATA_PACKET_BUFFER_SIZE     10

typedef struct dataPacket
{
    uint8_t device_id;
    uint8_t device_working_status;
    uint32_t device_message_counter;
    char device_type[19];
    char device_description[500];
    char message_date_time[24];
    double latitude;
    double longitude;
    double altitude;
    double temp_internal;
    double temp;
    double humidity;
    double co2;
    double tvoc;
    double pressure;
    double co;
    double co_temp;
    double co_hum;
    double no2;
    double no2_temp;
    double no2_hum;
    double so2;
    double so2_temp;
    double so2_hum;
    double o3;
    double o3_temp;
    double o3_hum;
    double hcho;
    double pm2_5;
    double pm10;
} dataPacket_S;

#endif
