#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#define DATA_PACKET_BUFFER_SIZE     10

typedef struct dataPacket
{
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
