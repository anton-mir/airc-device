#include "flash_SST25VF016B.h"
#include "config_board.h"
void InvertUint16(unsigned short *dBuf,unsigned short *srcBuf)
{
    int i;
    unsigned short tmp[4];
    tmp[0] = 0;
    for(i=0;i< 16;i++)
    {
        if(srcBuf[0]& (1 << i))
            tmp[0]|=1<<(15 - i);
    }
    dBuf[0] = tmp[0];
}

void InvertUint8(unsigned char *dBuf,unsigned char *srcBuf)
{
    int i;
    unsigned char tmp[4];
    tmp[0] = 0;
    for(i=0;i< 8;i++)
    {
        if(srcBuf[0]& (1 << i))
            tmp[0]|=1<<(7-i);

    }
    dBuf[0] = tmp[0];
}

unsigned short CRC16_CCITT(unsigned char *puchMsg, unsigned int usDataLen)
{
    unsigned short wCRCin = 0x0000;
    unsigned short wCPoly = 0x1021;
    unsigned char wChar = 0;

    int i;
    while (usDataLen--)
    {
        wChar = *(puchMsg++);
        InvertUint8(&wChar,&wChar);
        wCRCin ^= (wChar << 8);
        for(i = 0;i < 8;i++)
        {
            if(wCRCin & 0x8000)
                wCRCin = (wCRCin << 1) ^ wCPoly;
            else
                wCRCin = wCRCin << 1;
        }
    }
    InvertUint16(&wCRCin,&wCRCin);
    return (wCRCin) ;
}

void WriteConfig(boxConfig_S cfg)
{
    boxConfig_CRC16_CCITT_S cfg_crc;
    cfg_crc.cfg=cfg;
    cfg_crc.CRC16_CCITT=CRC16_CCITT(&cfg,sizeof(boxConfig_S));
    WriteToStatusRegister(0);
    ClearSector(ADDRESS_CFG_START);
    WriteDataArrayWithAAI(ADDRESS_CFG_START,&cfg_crc,sizeof(boxConfig_CRC16_CCITT_S));
    WriteToStatusRegister(0);
    ClearSector(ADDRESS_CFG_START_RESERV);
    WriteDataArrayWithAAI(ADDRESS_CFG_START_RESERV,&cfg_crc,sizeof(boxConfig_CRC16_CCITT_S));
    return;
}

void ReadConfig(boxConfig_S* cfg)
{
    boxConfig_CRC16_CCITT_S cfg_crc;
    ReadDataArrayFromAddress(ADDRESS_CFG_START,&cfg_crc,sizeof(boxConfig_CRC16_CCITT_S));
    if(cfg_crc.CRC16_CCITT==CRC16_CCITT(&(cfg_crc.cfg),sizeof(boxConfig_S)))
    {
        *(cfg)=cfg_crc.cfg;
        return;
    }else
    {
        ReadDataArrayFromAddress(ADDRESS_CFG_START_RESERV,&cfg_crc,sizeof(boxConfig_CRC16_CCITT_S));
        if(cfg_crc.CRC16_CCITT==CRC16_CCITT(&(cfg_crc.cfg),sizeof(boxConfig_S)))
        {
            *(cfg)=cfg_crc.cfg;
            return;
        }else{
            return;
        }
    }
}
