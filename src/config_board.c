#include "flash_SST25VF016B.h"
#include "config_board.h"

void WriteConfig(boxConfig cfg)
{

	ClearSector(ADDRESS_CFG_START);
	WriteDataArrayWithAAI(ADDRESS_CFG_START,&cfg,sizeof(boxConfig));
	return;
}

boxConfig ReadConfig(void)
{
	boxConfig cfg_buffer={};
	ReadDataArrayFromAddress(ADDRESS_CFG_START,&cfg_buffer,sizeof(boxConfig));
	return cfg_buffer;
}
