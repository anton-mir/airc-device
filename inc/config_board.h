#ifndef INC_CONFIG_BOARD_H_
#define INC_CONFIG_BOARD_H_

#include "main.h"

#define ADDRESS_CFG_START 0x000000
#define ADDRESS_CFG_START_RESERV 0x001000

typedef struct boxConfig
{
    double latitude;
    double longitude;
    double altitude;
    unsigned long long int SO2_specSN;
    unsigned long long int NO2_specSN;
    unsigned long long int CO_specSN;
    unsigned long long int O3_specSN;
    uint8_t id;
    uint8_t working_status;
    char description[500];
    char wifi_pass[64];
    char fb_pass[64];
    char wifi_ssid[32];
    char fb_email[32];
    char type[19];
    char ip[15];
    unsigned long long int sent_packet_counter;
}boxConfig_S;

typedef struct boxConfig_CRC16_CCITT
{
    boxConfig_S cfg;
    unsigned short CRC16_CCITT;
}boxConfig_CRC16_CCITT_S;


void WriteConfig(boxConfig_S cfg);
void ReadConfig(boxConfig_S* cfg);

#endif /* INC_CONFIG_BOARD_H_ */
