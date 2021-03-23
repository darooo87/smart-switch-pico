#ifndef WIFI_MQTT_LAYER_H__
#define WIFI_MQTT_LAYER_H__

enum MessageType {
    Pong,
    TurnOn,
    TurnOff,
    DeviceState,
    Other
};

int mqtt_initialize(
    int (*read_data)(unsigned char *data, size_t len),
    int (*write_data)(const unsigned char *data, size_t len));

int mqtt_connect();

int mqtt_subscribe();

enum MessageType mqtt_read_message();

int mqtt_send_message(const unsigned char *message, size_t len);

int mqtt_data_received(void *sck, unsigned char *buf, int count);

int mqtt_fill_buffer(unsigned char *buf, int len);

#endif