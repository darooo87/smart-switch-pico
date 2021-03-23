#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#include <stdio.h>

#include "config.h"
#include "tcp_layer.h"
#include "mqtt_layer.h"
#include "ssl_layer.h"

#define BAUD_RATE 115200

bool SendMqttKeepAlive = false;
bool SendMqttDeviceState = false;

struct repeating_timer timer;

bool send_keep_alive(struct repeating_timer *t);
void board_initialize();

int main()
{
    const char *host = IOT_HUB;
    const char *port = IOT_HUB_PORT;

    board_initialize();

    tcp_initialize();

    int connection_id = tcp_connect(host, port);

    ssl_connect(host, tcp_read, tcp_write, &connection_id);

    mqtt_initialize(ssl_read, ssl_write);
    mqtt_connect();
    mqtt_subscribe();

    add_repeating_timer_ms(IOT_KEEPALIVE_INTERVAL * 1000, send_keep_alive, NULL, &timer);

    while (true)
    {
        if (tcp_received_data())
        {
            enum MessageType message = mqtt_read_message();

            switch (message)
            {
            case Pong:
                printf("PONG\n");
                break;
            case TurnOn:
                printf("Turn On\n");
                break;
            case TurnOff:
                printf("Turn Off\n");
                break;
            }
        }
        else if (SendMqttKeepAlive)
        {
            mqtt_send_ping();

            SendMqttKeepAlive = false;
        }
        else if (SendMqttDeviceState)
        {
            char *message = "{ \"device-state\": \"on\" }";

            mqtt_send_message(message, sizeof(message));
        }

        sleep_ms(50);
    }
}

bool send_keep_alive(struct repeating_timer *t)
{
    SendMqttKeepAlive = true;

    return true;
}

void board_initialize()
{
    stdio_init_all();

    printf("\r\n\r\n*** program start ***\r\n\r\n");

    for (uint i = 0; i < 10; i++)
    {
        printf(".");
        sleep_ms(1000);
    }

    uart_init(uart1, BAUD_RATE);
    gpio_set_function(8, GPIO_FUNC_UART);
    gpio_set_function(9, GPIO_FUNC_UART);

    uart_set_fifo_enabled(uart1, false);
    uart_set_hw_flow(uart1, false, false);

    irq_set_exclusive_handler(UART1_IRQ, uart_data_received_handler);
    irq_set_enabled(UART1_IRQ, true);

    uart_set_irq_enables(uart1, true, false);

    printf("\r\n");
}