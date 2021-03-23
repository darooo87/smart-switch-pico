#include <string.h>
#include "pico/types.h"
#include "MQTTPacket.h"
#include "config.h"
#include "mqtt_layer.h"

unsigned char mqtt_read_buffer[500];
unsigned char *mqtt_read_buffer_read_position = mqtt_read_buffer;
unsigned char *mqtt_read_buffer_write_position = mqtt_read_buffer;

unsigned char mqtt_write_buffer[400];
int mqtt_write_buffer_length = sizeof(mqtt_write_buffer);

uint mqqtReadPosition = 0;
uint mqqtReadLength = 0;

MQTTPacket_connectData mqttData = MQTTPacket_connectData_initializer;
MQTTString subscribeTopicString = MQTTString_initializer;
MQTTString publishTopicString = MQTTString_initializer;
MQTTTransport mytransport;

int rc = 0;
int mysock = 4;
int len = 0;
int state;
int msgid = 1;
int req_qos = 0;

int (*connection_read)(unsigned char *data, size_t len);
int (*connection_write)(const unsigned char *data, size_t len);

int mqtt_initialize(
    int (*read_data)(unsigned char *data, size_t len),
    int (*write_data)(const unsigned char *data, size_t len))
{
    connection_read = read_data;
    connection_write = write_data;

    subscribeTopicString.cstring = IOT_SUBSCRIBE_TOPIC ;
    publishTopicString.cstring = IOT_PUBLISH_TOPIC ;

    mytransport.sck = &mysock;
    mytransport.getfn = mqtt_data_received;
    mytransport.state = 0;
    mqttData.clientID.cstring = IOT_CLIENT_ID;
    mqttData.keepAliveInterval = IOT_KEEPALIVE_INTERVAL;
    mqttData.cleansession = 1;
    mqttData.username.cstring = IOT_USER_NAME;
    mqttData.password.cstring = IOT_PASSWORD;

    return 0;
}

int mqtt_connect()
{
    int len = MQTTSerialize_connect(mqtt_write_buffer, mqtt_write_buffer_length, &mqttData);

    connection_write(mqtt_write_buffer, len);

    uint32_t loopNumber = 0;

    int rlen;
    unsigned char tmp[512];

    rlen = connection_read(tmp, sizeof tmp);

    mqtt_fill_buffer(tmp, rlen);

    do
    {
        int frc;

        if ((frc = MQTTPacket_readnb(tmp, rlen, &mytransport)) == CONNACK)
        {
            unsigned char sessionPresent, connack_rc;
            if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, tmp, rlen) != 1 || connack_rc != 0)
            {
                //printf("Unable to connect, return code %d\n", connack_rc);

                return -1;
            }

            break;
        }
        else if (frc == -1)
        {
            //printf("MQTT FRC -1\r\n");

            return -1;
        }

    } while (1);

    //printf("MQTT connected\n");
}

int mqtt_subscribe()
{
    int len = MQTTSerialize_subscribe(mqtt_write_buffer, mqtt_write_buffer_length, 0, msgid, 1, &subscribeTopicString, &req_qos);

    connection_write(mqtt_write_buffer, len);

    int rlen;
    unsigned char tmp[512];

    rlen = connection_read(tmp, sizeof tmp);

    mqtt_fill_buffer(tmp, rlen);

    do
    {
        int frc;

        if ((frc = MQTTPacket_readnb(tmp, rlen, &mytransport)) == SUBACK)
        {
            unsigned short submsgid;
            int subcount;
            int granted_qos;

            rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, tmp, rlen);

            if (granted_qos != 0)
            {
                //printf("granted qos != 0, %d\n", granted_qos);
                
                return -1;
            }
            break;
        }
        else if (frc == -1)
        {
            //printf("MQTT FRC -1\r\n");

            return -1;
        }
    } while (1);

    //printf("Subscribed\n");
}

int mqtt_send_ping()
{
    int len = MQTTSerialize_pingreq(mqtt_write_buffer, mqtt_write_buffer_length);

    connection_write(mqtt_write_buffer, len);
}

enum MessageType mqtt_read_message()
{
    unsigned char tmp[512];

    int rlen = connection_read(tmp, sizeof tmp);

    mqtt_fill_buffer(tmp, rlen);

    int rc = MQTTPacket_readnb(tmp, rlen, &mytransport);

    if (rc == PINGRESP)
    {
        return Pong;
    }
    else if (rc == PUBLISH)
    {
        int qos;
        int payloadlen_in;
        unsigned char dup;
        unsigned char retained;
        unsigned short msgid;
        unsigned char *payload_in;

        MQTTString receivedTopic;

        rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic, &payload_in, &payloadlen_in, tmp, rlen);

        if (strncmp(payload_in, "turn-on", payloadlen_in) == 0)
        {
            return TurnOn;
        }
        else if (strncmp(payload_in, "turn-off", payloadlen_in) == 0)
        {
            return TurnOff;
        }
        else if (strncmp(payload_in, "get-state", payloadlen_in) == 0)
        {
            return DeviceState;
        }
        else
        {
            return Other;
        }
    }
    else
    {
        return Other;
    }
}

int mqtt_send_message(const unsigned char *message, size_t len)
{
    int rlen = MQTTSerialize_publish(mqtt_write_buffer, mqtt_write_buffer_length, 0, 0, 0, 0, publishTopicString, (unsigned char*)message, len);

    connection_write(mqtt_write_buffer, rlen);
}

int mqtt_fill_buffer(unsigned char *buf, int len)
{
    mqqtReadLength += len;

    memcpy(mqtt_read_buffer_write_position, buf, len);
}

int mqtt_data_received(void *sck, unsigned char *buf, int count)
{
    if (count < mqqtReadLength)
    {
        memcpy(buf, mqtt_read_buffer_read_position, count);

        mqqtReadLength = mqqtReadLength - count;

        mqtt_read_buffer_read_position = mqtt_read_buffer_read_position + count;

        return count;
    }
    else
    {
        memcpy(buf, mqtt_read_buffer_read_position, mqqtReadLength);

        int output = mqqtReadLength;

        mqqtReadLength = 0;
        mqtt_read_buffer_read_position = &mqtt_read_buffer[0];
        mqtt_read_buffer_write_position = &mqtt_read_buffer[0];

        return output;
    }
}