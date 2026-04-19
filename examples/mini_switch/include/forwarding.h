#pragma once
#include <stdint.h>

void forward_init(void);
void forward_frame(packet_t *pkt, int ingress_port);
void tx_flood_exclude(packet_t *pkt, int exclude_port);
void tx_send(int port, const uint8_t *buf, size_t len);
