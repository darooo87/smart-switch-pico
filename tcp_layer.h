#ifndef WIFI_TCP_LAYER_H__
#define WIFI_TCP_LAYER_H__

#include "pico/types.h"

int tcp_initialize();
int tcp_connect(const unsigned char* host, const unsigned char* port);
int tcp_read(void *ctx, unsigned char *buf, size_t len);
int tcp_write(void *ctx, const unsigned char *buf, size_t len);

bool tcp_received_data();
void uart_data_received_handler();

#endif