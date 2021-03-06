#ifndef MQTTHELPER_H_
#define MQTTHELPER_H_

#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <PubSubClient.h>
#include <Preferences.h>
#include "MDNSHelper.h"

#define MQTT_HOST_DEFAULT           "hassistant"
#define MQTT_TOPIC_PREFIX_DEFAULT   "esp32"
#define INVALID_IP                  "0.0.0.0"

class MQTTHelperClass
{
    Preferences * m_Preferences;
    MDNSHelper * m_DnsHelper;
    String m_MqttHost;
    String m_MqttHostIP = INVALID_IP;
    String m_MqttTopicPrefix;
    WiFiClient espClient;
    PubSubClient * m_MqttClient;
    bool m_IsConnected = false;
    unsigned long m_LastReconnectRetry = 0;
    std::function<void(char*, uint8_t*, unsigned int)> m_Callback;
    Stream * m_Logger = NULL;
private:
    static String processor(const String& var);
    void tryConnect();
    bool setServer(const String& hostname);
    bool isValidIP(const String& ipAddr);
public:
    void begin(Preferences * preferences, MDNSHelper * dnsHelper, Stream * logger, MQTT_CALLBACK_SIGNATURE);
    void bind(AsyncWebServer * server);
    void publish(const char * subTopic, const char * data);
    void publish(const char * subTopic, const char * data, bool retain);
    static void factoryReset(MQTTHelperClass * mqttHelper);
    bool poll();
    String getHost() {return m_MqttHost;}
    String getTopicPrefix() {return m_MqttTopicPrefix;}
    bool isConnected() { return m_IsConnected;}
};

extern MQTTHelperClass MQTTHelper;

#endif