
#ifndef DATA_COLLECTOR_H
#define DATA_COLLECTOR_H
#include "data_structure.h"
/*
    DATA_PACKET_BUFFER_SIZE defines how frequent the fans will be turn on/off.
    Because we add one packet to the buffer every second until it`s get full.
*/
#define DATA_PACKET_BUFFER_SIZE     10

void data_collector(void *pvParameters);
void avrg_data_packets(dataPacket_S *buffer, int buf_size, dataPacket_S *result_packet);
#endif
