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
        request->send(SPIFFS, "/mqtt/config.html", "text/html", false, processor);
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
    Serial.printf("Publish: [%s] <= [%s]\n", subTopic, data);

    int totalLength = MQTT_MAX_HEADER_SIZE + 3 + strlen(subTopic) + strlen(data) + m_MqttTopicPrefix.length();

    if (MQTT_MAX_PACKET_SIZE > totalLength)
    {
        m_MqttClient->publish((m_MqttTopicPrefix + "/" + subTopic).c_str(), data, retain);
    }
    else
    {
        m_MqttClient->beginPublish((m_MqttTopicPrefix + "/" + subTopic).c_str(), strlen(data), retain);
        m_MqttClient->print(data);
        m_MqttClient->endPublish();
    }
}

void MQTTHelperClass::tryConnect()
{
    // Loop until we're reconnected
    if (millis() - m_LastReconnectRetry > 5000)
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        m_MqttClient->setServer(m_MqttHostIP.c_str(), 1883);
        if (m_MqttClient->connect(WiFi.macAddress().c_str(), 
            (m_MqttTopicPrefix + "/status").c_str(), 0, true, "offline")) 
        {
            Serial.println("connected");
            // Subscribe
            m_MqttClient->subscribe((m_MqttTopicPrefix + "/+/+/command").c_str());
            m_MqttClient->subscribe((m_MqttTopicPrefix + "/+/command").c_str());
            m_MqttClient->publish((m_MqttTopicPrefix + "/status").c_str(), "online", true);
        } 
        else 
        {
            Serial.print("failed, rc=");
            Serial.print(m_MqttClient->state());
            Serial.println(" try again in 5 seconds");
            m_LastReconnectRetry = millis();
        }
    }
}



bool MQTTHelperClass::poll()
{
    // TODO make this unblocking
    if (!isValidIP(m_MqttHostIP))
    {
        setServer(m_MqttHost);
    }

    if (isValidIP(m_MqttHostIP) && !m_MqttClient->connected())
    {
        m_IsConnected = false;
        tryConnect();
    }

    if (isValidIP(m_MqttHostIP) && m_MqttClient->connected()) 
    {
        m_IsConnected = true;
        m_MqttClient->loop();
    }
}

bool MQTTHelperClass::isValidIP(const String& ipAddr)
{
    return ipAddr != INVALID_IP;
}

MQTTHelperClass MQTTHelper;