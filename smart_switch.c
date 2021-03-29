#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#include "config.h"
#include "tcp_layer.h"
#include "ssl_layer.h"
#include "mqtt_layer.h"

#define BAUD_RATE 115200
#define RELAY_PIN 22
#define INFO_LED_PIN 25

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

    uart_puts(uart1, "board initialize done\r\n");

    tcp_initialize();

    uart_puts(uart1, "tcp initialize done\r\n");

    int connection_id = tcp_connect(host, port);

    uart_puts(uart1, "tcp connected\r\n");

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
                uart_puts(uart1, "PONG\r\n");
                gpio_put(INFO_LED_PIN, 1);
                sleep_ms(500);
                gpio_put(INFO_LED_PIN, 0);
                break;
            case TurnOn:
                uart_puts(uart1, "Turn On\r\n");
                gpio_put(RELAY_PIN, 1);
                break;
            case TurnOff:
                uart_puts(uart1, "Turn Off\r\n");
                gpio_put(RELAY_PIN, 0);
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
    uart_init(uart1, BAUD_RATE);
    gpio_set_function(4, GPIO_FUNC_UART);
    gpio_set_function(5, GPIO_FUNC_UART);

    sleep_ms(500);
    
    uart_puts(uart1, "\r\n\r\n*** program start ***\r\n\r\n");

    for (uint i = 0; i < 10; i++)
    {
        uart_puts(uart1, ".");
        sleep_ms(1000);
    }

    uart_puts(uart1, "\r\n\r\n");

    uart_init(uart0, BAUD_RATE);
    gpio_set_function(16, GPIO_FUNC_UART);
    gpio_set_function(17, GPIO_FUNC_UART);

    gpio_init(RELAY_PIN);
    gpio_set_dir(RELAY_PIN, GPIO_OUT);

    gpio_init(INFO_LED_PIN);
    gpio_set_dir(INFO_LED_PIN, GPIO_OUT);

    uart_set_fifo_enabled(uart0, false);
    uart_set_hw_flow(uart0, false, false);

    irq_set_exclusive_handler(UART0_IRQ, uart_data_received_handler);

    irq_set_enabled(UART0_IRQ, true);

    uart_set_irq_enables(uart0, true, false);
}