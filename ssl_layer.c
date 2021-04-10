#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "ssl_layer.h"
#include "ssl_trusted_anchors.h"

#include "bearssl.h"


br_sslio_context ssl_io_context;
br_ssl_client_context ssl_client_context;
br_x509_minimal_context ssl_x509_context;

unsigned char tls_io_buf[BR_SSL_BUFSIZE_BIDI];

int ssl_connect(
    const unsigned char *host,
    int (*tcp_read)(void *read_context, unsigned char *data, size_t len), 
    int (*tcp_write)(void *write_context, const unsigned char *data, size_t len),
    void *connection_id)
{
    uint8_t rng_seeds[16];

    adc_gpio_init(26);
    adc_select_input(0);

    for (uint8_t i = 0; i < sizeof rng_seeds; i++){
        rng_seeds[i] = adc_read();
    }

    br_ssl_client_init_full(&ssl_client_context, &ssl_x509_context, TAs, TAs_NUM);
    br_ssl_engine_set_buffer(&ssl_client_context.eng, tls_io_buf, sizeof tls_io_buf, 1);
    br_ssl_engine_inject_entropy(&ssl_client_context.eng, rng_seeds, sizeof rng_seeds);
    br_ssl_client_reset(&ssl_client_context, host, 0);
    br_sslio_init(&ssl_io_context, &ssl_client_context.eng, tcp_read, &connection_id, tcp_write, &connection_id);

    br_x509_minimal_set_time(&ssl_x509_context, 738231, 32400);

    return br_sslio_flush(&ssl_io_context);
}

int ssl_write(const unsigned char *src, size_t len)
{
    br_sslio_write_all(&ssl_io_context, src, len);
    br_sslio_flush(&ssl_io_context);
}

int ssl_read(unsigned char *dst, size_t size){
    return br_sslio_read(&ssl_io_context, dst, size);
}