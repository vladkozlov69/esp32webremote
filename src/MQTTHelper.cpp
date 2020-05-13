#include "MQTTHelper.h"

void MQTTHelperClass::begin(Preferences * preferences, MDNSHelper * dnsHelper, MQTT_CALLBACK_SIGNATURE)
{
    m_Preferences = preferences;
    m_DnsHelper = dnsHelper;
    m_Callback = callback;

    m_Preferences->begin("mqtt", false);
    m_MqttHost = m_Preferences->getString("mqttServer", MQTT_HOST_DEFAULT);
    m_MqttTopicPrefix = m_Preferences->getString("mqttTopic", MQTT_TOPIC_PREFIX_DEFAULT);
    m_Preferences->end();

    m_MqttClient = new PubSubClient(espClient);

    // try to init mqtt
    setServer(m_MqttHost);
    Serial.println("MQTT Config done");
}

bool MQTTHelperClass::setServer(const String& hostname)
{
    m_MqttHostIP = INVALID_IP;
    String mqtt_ip = m_DnsHelper->resolve(hostname.c_str());
    if (isValidIP(mqtt_ip))
    {
        m_MqttHostIP = mqtt_ip;
        Serial.print("Setting MQTT IP:");
        Serial.println(mqtt_ip);
        m_MqttClient->setServer(mqtt_ip.c_str(), 1883);
        Serial.print("Setting callback... ");
        m_MqttClient->setCallback(m_Callback);
        Serial.println(" - OK");
    }
}

void MQTTHelperClass::factoryReset(MQTTHelperClass * mqttHelper)
{
    mqttHelper->m_MqttHost = MQTT_HOST_DEFAULT;
    mqttHelper->m_MqttTopicPrefix = MQTT_TOPIC_PREFIX_DEFAULT;

    mqttHelper->m_Preferences->begin("mqtt", false);
    mqttHelper->m_Preferences->putString("mqttServer", mqttHelper->m_MqttHost);
    mqttHelper->m_Preferences->putString("mqttTopic", mqttHelper->m_MqttTopicPrefix);
    mqttHelper->m_Preferences->end();
}

String MQTTHelperClass::processor(const String& var)
{
    if(var == "mqtthost") return MQTTHelper.m_MqttHost;
    if(var == "mqttprfx") return MQTTHelper.m_MqttTopicPrefix;

    return String();
}

void MQTTHelperClass::bind(AsyncWebServer * server)
{
    server->on("/mqtt/config", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/mqtt/config.html", String(), false, processor);
    });

    server->on("/mqtt/factory", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        factoryReset(&MQTTHelper);

        request->redirect("/mqtt/config");
    });

    server->on("/mqtt/save", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        MQTTHelper.m_MqttHost = request->getParam("mqtthost", true, false)->value();
        MQTTHelper.m_MqttTopicPrefix = request->getParam("mqttprfx", true, false)->value();

        MQTTHelper.m_Preferences->begin("mqtt", false);
        MQTTHelper.m_Preferences->putString("mqttServer", MQTTHelper.m_MqttHost);
        MQTTHelper.m_Preferences->putString("mqttTopic", MQTTHelper.m_MqttTopicPrefix);
        MQTTHelper.m_Preferences->end();
        
        request->redirect("/mqtt/config");
    });
}

void MQTTHelperClass::publish(const char * subTopic, const char * data)
{
    publish(subTopic, data, false);
}

void MQTTHelperClass::publish(const char * subTopic, const char * data, bool retain)
{
    m_MqttClient->publish((m_MqttTopicPrefix + "/" + subTopic).c_str(), data, retain);
}

void MQTTHelperClass::reconnect()
{
    // Loop until we're reconnected
    while (!m_MqttClient->connected()) 
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        m_MqttClient->setServer(m_MqttHostIP.c_str(), 1883);
        if (m_MqttClient->connect(WiFi.macAddress().c_str())) 
        {
            Serial.println("connected");
            // Subscribe
            m_MqttClient->subscribe((m_MqttTopicPrefix + "/+/+/command").c_str());
            m_MqttClient->subscribe((m_MqttTopicPrefix + "/+/command").c_str());
        } 
        else 
        {
            Serial.print("failed, rc=");
            Serial.print(m_MqttClient->state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

bool MQTTHelperClass::poll()
{
    if (!isValidIP(m_MqttHostIP))
    {
        setServer(m_MqttHost);
    }

    if (isValidIP(m_MqttHostIP) && !m_MqttClient->connected())
    {
        reconnect();
    }

    if (isValidIP(m_MqttHostIP) && m_MqttClient->connected()) 
    {
        m_MqttClient->loop();
    }
}

bool MQTTHelperClass::isValidIP(const String& ipAddr)
{
    return ipAddr != INVALID_IP;
}

MQTTHelperClass MQTTHelper;