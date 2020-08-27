#include "flash_SST25VF016B.h"
#include "config_board.h"

void WriteConfig(boxConfig_S cfg)
{

	ClearSector(ADDRESS_CFG_START);
	WriteDataArrayWithAAI(ADDRESS_CFG_START,&cfg,sizeof(boxConfig_S));
	return;
}

boxConfig_S ReadConfig(void)
{
    boxConfig_S cfg_buffer;
	ReadDataArrayFromAddress(ADDRESS_CFG_START,&cfg_buffer,sizeof(boxConfig_S));
	return cfg_buffer;
}
