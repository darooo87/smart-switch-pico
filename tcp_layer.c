#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "hardware/uart.h"
#include "pico/time.h"

#include "config.h"
#include "tcp_layer.h"

bool esp01_at_ok_received = false;
bool esp01_at_error_received = false;
bool esp01_receiving_data = false;
bool esp01_receiving_header = false;

char tmp_buffer[200];
uint tmp_buffer_length = sizeof(tmp_buffer);
char *tmp_buffer_position = tmp_buffer;

unsigned char data_buffer[2000];
unsigned char *data_buffer_position = &data_buffer[0];
uint dataLength = 0;
uint dataPosition = 0;

unsigned char read_buffer[8000];
unsigned char *read_buffer_write_position = &read_buffer[0];
unsigned char *read_buffer_read_position = &read_buffer[0];
uint readLength = 0;
uint readPosition = 0;

#define UART_WRITE_SUCCESS 0
#define UART_WRITE_TIME_EXCEEDED -1
#define UART_WRITE_ERROR -2

int esp01_send_at_command(const unsigned char *command)
{
#ifdef AT_DEBUG
    uart_puts(uart1, "AT - send at command start \r\n");
#endif

    uint i = 0;

    while (true)
    {
        if (++i > 100)
            return UART_WRITE_TIME_EXCEEDED;

        if (uart_is_writable(uart0))
            break;

        sleep_ms(10);
    }

    if (uart_is_writable(uart0))
    {
        uart_puts(uart0, command);
    }

    while (!esp01_at_ok_received && !esp01_at_error_received)
    {
        sleep_ms(5);
    }

    if (esp01_at_error_received)
    {
#ifdef AT_DEBUG
        uart_puts(uart1, "AT - send at command returned error \r\n");
#endif
        esp01_at_ok_received = false;
        esp01_at_error_received = false;

        return UART_WRITE_ERROR;
    }

    esp01_at_ok_received = false;

#ifdef AT_DEBUG
    uart_puts(uart1, "AT - send at command complete \r\n");
#endif

    return UART_WRITE_SUCCESS;
}

int esp01_send_at_bytes(const unsigned char *command, size_t len)
{
#ifdef AT_DEBUG
    uart_puts(uart1, "AT - send at bytes command \r\n");
#endif
    uint i = 0;

    while (true)
    {
        if (++i > 100)
            return UART_WRITE_TIME_EXCEEDED;

        if (uart_is_writable(uart0))
            break;

        sleep_ms(10);
    }

    for (int i = 0; i < len;)
    {
        if (uart_is_writable(uart0))
        {
            uart_putc_raw(uart0, command[i]);
            i++;
        }
        else
        {
            sleep_ms(1);
        }
    }

    while (!esp01_at_ok_received && !esp01_at_error_received)
    {
        sleep_ms(5);
    }

    if (esp01_at_error_received)
    {
#ifdef AT_DEBUG
        uart_puts(uart1, "AT - send at bytes returned error \r\n");
#endif
        esp01_at_ok_received = false;
        esp01_at_error_received = false;

        return UART_WRITE_ERROR;
    }

    esp01_at_ok_received = false;

#ifdef AT_DEBUG
    uart_puts(uart1, "AT - send at bytes command complete \r\n");
#endif

    return UART_WRITE_SUCCESS;
}

int tcp_initialize()
{
    esp01_send_at_command("AT+CIPMUX=1\r\n");
}

int connect_to_wifi()
{

    esp01_send_at_command("AT+CWMODE=1\r\n");

    char atConnectCommand[100] = "AT+CWJAP=\"";
    strcat(atConnectCommand, WIFI_SSID);
    strcat(atConnectCommand, "\",\"");
    strcat(atConnectCommand, WIFI_PASSWORD);
    strcat(atConnectCommand, "\"\r\n");

    esp01_send_at_command("AT+CWMODE=1\r\n");
}

int tcp_connect(const unsigned char *host, const unsigned char *port)
{
#ifdef TCP_DEBUG
    uart_puts(uart1, "AT - TCP connect start \r\n");
#endif

    char atConnectCommand[100] = "AT+CIPSTART=4,\"TCP\",\"";
    strcat(atConnectCommand, host);
    strcat(atConnectCommand, "\",");
    strcat(atConnectCommand, port);
    strcat(atConnectCommand, "\r\n");

    esp01_send_at_command(atConnectCommand);

    return 4;

#ifdef TCP_DEBUG
    uart_puts(uart1, "AT - TCP connect complete \r\n");
#endif
}

bool tcp_received_data()
{
    return readLength > 0;
}

int tcp_read(void *ctx, unsigned char *buf, size_t len)
{
    while (readLength <= 0 || esp01_receiving_data == true)
    {
        sleep_ms(1);
    }

    if ((int)len < readLength)
    {
        memcpy(buf, read_buffer_write_position, (int)len);

        readLength = readLength - (int)len;

        read_buffer_write_position = read_buffer_write_position + (int)len;

        return (int)len;
    }
    else
    {
        memcpy(buf, read_buffer_write_position, readLength);

        int output = readLength;

        readLength = 0;
        read_buffer_write_position = &read_buffer[0];
        read_buffer_read_position = &read_buffer[0];

        return output;
    }
}

int tcp_write(void *ctx, const unsigned char *buf, size_t len)
{
    char dataLength[10];
    sprintf(dataLength, "%d", len);

    char connectionIdStr[10];
    sprintf(connectionIdStr, "%d", 4);

    char atStartData[100] = "AT+CIPSEND=4,";
    strcat(atStartData, dataLength);
    strcat(atStartData, "\r\n");

    if (esp01_send_at_command(atStartData) != UART_WRITE_SUCCESS)
        return UART_WRITE_ERROR;

    if (esp01_send_at_bytes(buf, len) != UART_WRITE_SUCCESS)
        return UART_WRITE_ERROR;

    return (int)len;
}

inline void clear_tmp_data_buffer()
{
    memset(tmp_buffer, 0, tmp_buffer_length);
    tmp_buffer_position = tmp_buffer;
}

void uart_data_received_handler()
{
    while (uart_is_readable(uart0))
    {
        *tmp_buffer_position = uart_getc(uart0);
    }

    if (esp01_receiving_data)
    {
        *data_buffer_position = *tmp_buffer_position;

        data_buffer_position++;

        if (++dataPosition == dataLength)
        {

#ifdef TCP_DEBUG
            //printf("received %d bytes\r\n", dataPosition);
            uart_puts(uart1, "AT - TCP connect complete \r\n");
#endif

            memcpy(read_buffer_read_position, &data_buffer[0], dataPosition);

            read_buffer_read_position = read_buffer_read_position + dataPosition;

            readLength += dataLength;

            data_buffer_position = &data_buffer[0];

            dataLength = 0;
            dataPosition = 0;

            esp01_receiving_data = false;
        }

        return;
    }

    if (tmp_buffer_position == tmp_buffer && *tmp_buffer_position == '+')
    {
        esp01_receiving_header = true;

        return;
    }

    if (esp01_receiving_header && *tmp_buffer_position == ':')
    {
        esp01_receiving_header = false;
        esp01_receiving_data = true;

        char *endPosition = strchr(tmp_buffer, ':');
        char *startPosition = tmp_buffer + 6;

        int length = endPosition - startPosition;

        char subbuff[length + 1];

        memcpy(subbuff, startPosition, length);
        subbuff[length] = '\0';

        dataLength = atoi(subbuff);
        dataPosition = 0;

        return;
    }

#ifdef AT_DEBUG
    if (esp01_receiving_data == false && esp01_receiving_header == false)
    {
        uart_putc(uart1, *tmp_buffer_position);
    }
#endif

    if (*tmp_buffer_position == '\n')
    {
        if (strstr(tmp_buffer, "OK\r\n") != NULL)
        {
            esp01_at_error_received = false;
            esp01_at_ok_received = true;
        }
        if (strstr(tmp_buffer, "ERROR\r\n") != NULL)
        {
            esp01_at_ok_received = false;
            esp01_at_error_received = true;
        }

        clear_tmp_data_buffer();

        return;
    }

    tmp_buffer_position++;
}
