#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H


typedef struct dataPacket
{
    uint8_t id;
    uint8_t working_status;
    uint32_t messageId;
    char description[500];
    char type[19];
    char messageDateTime[30];
    double temp;
    double humidity;
    double co2;
    double tvoc;
    double pressure;
    double co;
    double no2;
    double so2;
    double o3;
    double hcho;
    double pm2_5;
    double pm10;
    double latitude;
    double longitude;
    double altitude;
}dataPacket_S;


#endif
