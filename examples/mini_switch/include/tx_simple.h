#pragma once
#include "common.h"

int tx_send(int port, const uint8_t *data, size_t len);
int tx_flood_exclude(packet_t *pkt, int exclude_port);
