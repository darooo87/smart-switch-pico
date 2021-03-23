#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "hardware/uart.h"
#include "pico/time.h"

#include "tcp_layer.h"

bool esp01_at_ok_received = false;
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

void esp01_send_at_command(const unsigned char *command)
{
    uint i = 0;

    while (true)
    {
        if (uart_is_writable(uart1) || ++i > 100)
            break;

        sleep_ms(10);
    }

    if (uart_is_writable(uart1))
    {
        uart_puts(uart1, command);
    }

    while (esp01_at_ok_received != true)
    {
    }

    esp01_at_ok_received = false;
}

void esp01_send_at_bytes(const unsigned char *command, size_t len)
{
    uint i = 0;

    while (true)
    {
        if (uart_is_writable(uart1) || ++i > 100)
            break;

        sleep_ms(10);
    }

    for (int i = 0; i < len;)
    {
        if (uart_is_writable(uart1))
        {
            uart_putc_raw(uart1, command[i]);
            i++;
        }
        else
        {
            sleep_ms(1);
        }
    }

    while (esp01_at_ok_received != true)
    {
    }

    esp01_at_ok_received = false;

    //printf("bytes end\r\n");
}

int tcp_initialize()
{
    esp01_send_at_command("AT+CIPMUX=1\r\n");
}

int tcp_connect(const unsigned char *host, const unsigned char *port)
{
    char atConnectCommand[100] = "AT+CIPSTART=4,\"TCP\",\"";
    strcat(atConnectCommand, host);
    strcat(atConnectCommand, "\",");
    strcat(atConnectCommand, port);
    strcat(atConnectCommand, "\r\n");

    esp01_send_at_command(atConnectCommand);

    return 4;
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

        //printf("esp read a %d\r\n", (int)len);

        return (int)len;
    }
    else
    {
        memcpy(buf, read_buffer_write_position, readLength);

        int output = readLength;

        readLength = 0;
        read_buffer_write_position = &read_buffer[0];
        read_buffer_read_position = &read_buffer[0];

        //printf("esp read b %d\r\n", output);

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

    esp01_send_at_command(atStartData);
    esp01_send_at_bytes(buf, len);

    return (int)len;
}

inline void clear_tmp_data_buffer()
{
    memset(tmp_buffer, 0, tmp_buffer_length);
    tmp_buffer_position = tmp_buffer;
}

void uart_data_received_handler()
{
    while (uart_is_readable(uart1))
    {
        *tmp_buffer_position = uart_getc(uart1);
    }

    if (esp01_receiving_data)
    {
        *data_buffer_position = *tmp_buffer_position;

        data_buffer_position++;

        if (++dataPosition == dataLength)
        {
            //printf("received %d bytes\r\n", dataPosition);

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

    if (esp01_receiving_data == false && esp01_receiving_header == false)
    {
        printf("%c", *tmp_buffer_position);
    }

    if (*tmp_buffer_position == '\n')
    {
        if (strstr(tmp_buffer, "OK\r\n") != NULL)
        {
            esp01_at_ok_received = true;
        }

        clear_tmp_data_buffer();

        return;
    }

    tmp_buffer_position++;
}
