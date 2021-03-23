#ifndef WIFI_TLS_LAYER_H__
#define WIFI_TLS_LAYER_H__

#include "pico/types.h"

int ssl_connect(
    const unsigned char *host,
    int (*tcp_read)(void *read_context, unsigned char *data, size_t len),
    int (*tcp_write)(void *write_context, const unsigned char *data, size_t len),
    void *connection_id);

int ssl_write(const unsigned char *src, size_t len);

int ssl_read(unsigned char *dst, size_t size);

int mqtt_send_ping();

#endif
