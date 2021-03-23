#ifndef SMARTSWITCH_CONFIG_H__
#define SMARTSWITCH_CONFIG_H__

#define IOT_HUB "your-hub-name.azure-devices.net"
#define IOT_HUB_PORT "8883"
#define IOT_SUBSCRIBE_TOPIC "devices/your-device-name/messages/devicebound/#"
#define IOT_PUBLISH_TOPIC "devices/your-device-name/messages/events/"
#define IOT_CLIENT_ID "your-device-name"
#define IOT_USER_NAME "your-hub-name.azure-devices.net/your-device-name/?api-version=2018-06-30"
// Use Azure SAS Token as password, you can generate SAS token using azure Cloud Shell:
// az iot hub generate-sas-token --device-id your-device-name --hub-name your-hub-name --duration 31536000
#define IOT_PASSWORD ""
#define IOT_KEEPALIVE_INTERVAL 20

#endif