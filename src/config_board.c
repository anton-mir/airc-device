#include "flash_SST25VF016B.h"
#include "config_board.h"

void WriteConfig(boxConfig_S cfg)
{
    WriteToStatusRegister(0);
    ClearSector(ADDRESS_CFG_START);
    WriteDataArrayWithAAI(ADDRESS_CFG_START,((uint8_t*)&cfg),sizeof(boxConfig_S));
    return;
}

void ReadConfig(boxConfig_S* cfg)
{
    ReadDataArrayFromAddress(ADDRESS_CFG_START,(uint8_t*)cfg,sizeof(boxConfig_S));
    return;
}
