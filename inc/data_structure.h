#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

typedef struct temperautre
{
    double field;
}temperautreS;

typedef struct humidity
{
    double field;

}humidityS;

typedef struct co2
{
    double field;

}co2S;

typedef struct tvoc
{
    double field;

}tvocS;

typedef struct pressure
{
    double field;

}pressureS;

typedef struct co
{
    double field;
}coS;

typedef struct no2
{
    double field;

}no2S;

typedef struct so2
{
    double field;

}so2S;

typedef struct o3
{
    double field;

}o3S;

typedef struct hcho
{
    double field;

}hchoS;

typedef struct pm2_5
{
    double field;

}pm2_5S;

typedef struct pm10
{
    double field;

}pm10S;


typedef struct dataPacket
{
    temperautreS temp;
    humidityS humidity;
    co2S co2;
    tvocS tvoc;
    pressureS pressure;
    coS co;
    no2S no2;
    so2S so2;
    o3S o3;
    hchoS hcho;
    pm2_5S pm2_5;
    pm10S pm10;
}dataPacket_S;

#endif
