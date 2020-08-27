#ifndef INC_CONFIG_BOARD_H_
#define INC_CONFIG_BOARD_H_
#define ADDRESS_CFG_START 0x000000

typedef struct boxConfig
{
    uint8_t id;
    char type[20];
    char description[500];
    double latitude;
    double longitude;
    double altitudeSet;
    uint8_t working_status;
}boxConfig_S;


void WriteConfig(boxConfig_S cfg);
void ReadConfig(boxConfig_S* cfg);

#endif /* INC_CONFIG_BOARD_H_ */
