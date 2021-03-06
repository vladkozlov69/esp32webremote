#include "RFPlugin.h"

String RFPluginClass::processor(const String& var)
{
    if(var == "sensorId") return String(RFPlugin.m_RegisteredSensorId);
    if(var == "ready") return RFPlugin.m_SensorRegisterMode ? "false" : "true";

    return String();
}

String RFPluginClass::getSensorsJson(RFPluginClass * rfPlugin)
{
    DynamicJsonDocument doc(4096);

    JsonArray acl = doc.createNestedArray("acl");

    for(int i = 0; i < rfPlugin->m_RegisteredSensors.length(); i++)
    {
        String e = *(rfPlugin->m_RegisteredSensors.nth(i));
        acl.add(e);
    }

    String resultJson = "";
    serializeJson(doc, resultJson);

    return resultJson;
}

bool RFPluginClass::begin(MQTTHelperClass * mqttHelper, int receivePin, int sendPin, Stream * logger)
{
    m_MQTTHelper = mqttHelper;
    m_RCSwitch.enableTransmit(sendPin);
    m_RCSwitch.enableReceive(receivePin);
    m_Logger = logger;

    File file = SPIFFS.open("/config/rf.json", "r");
    if (!file)
    {
        m_Logger->println("No RF ACL defined");
    } 
    else 
    {
        DynamicJsonDocument doc(file.size() + 512);
        DeserializationError result = deserializeJson(doc, file);

        if (DeserializationError::Ok == result.code())
        {
            JsonArray acl = doc["acl"];

            for (int i = 0; i < acl.size(); i++)
            {
                String elem = acl.getElement(i).as<char*>();
                m_RegisteredSensors.add(elem);
            }
        }
        else
        {
            m_Logger->print(String("RF config Deserialization error ") + result.c_str());

            return false;
        }
        
        file.close();
    }

    return true;
}

void RFPluginClass::enterRegistrationMode()
{
    m_SensorRegisterMode = true;
    m_RegisterModeStart = millis();
    m_RegisteredSensorId = 0;
}

void RFPluginClass::exitRegistrationMode()
{
    m_SensorRegisterMode = false;
    m_RegisterModeStart = 0;
    m_RegisteredSensorId = 0;
}

void RFPluginClass::bind(AsyncWebServer * server)
{
    server->on("/rf/index", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/rf/index.html", "text/html", false, processor);
    });

    server->on("/rf/list.json", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", 
            RFPlugin.getSensorsJson(&RFPlugin));
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    server->on("/rf/save", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        File file = SPIFFS.open("/config/rf.json", "w");
        file.print(RFPlugin.getSensorsJson(&RFPlugin));
        file.close();
        
        request->redirect("/rf/index");
    });

    server->on("/rf/remove", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String id = request->getParam("id")->value();

        RFPlugin.m_RegisteredSensors.remove(id);
        
        request->redirect("/rf/index");
    });

    server->on("/rf/register", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        RFPlugin.enterRegistrationMode();
        request->send(SPIFFS, "/rf/register.html", "text/html", false, processor);
    });

    server->on("/rf/cancel", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        RFPlugin.exitRegistrationMode();
        request->redirect("/rf/index");
    });

    server->on("/rf/confirm", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        char buf[16];
        ltoa(RFPlugin.m_RegisteredSensorId, buf, 10);

        if (!RFPlugin.m_RegisteredSensors.containsIgnoreCase(buf))
        {
            RFPlugin.m_RegisteredSensors.add(buf);
        }

        RFPlugin.exitRegistrationMode();

        request->send(SPIFFS, "/rf/confirm.html", "text/html", false, processor);
    });

    server->on("/rf/state.json", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/rf/state.json", "application/json", false, processor);
    });
}

void RFPluginClass::callback(const char* topic, const char * message)
{
    MatchState ms;
    ms.Target((char *)topic);

    if (REGEXP_MATCHED == ms.Match((MQTTHelper.getTopicPrefix() + "/rc/(%d+)/command").c_str()))
    {
        char buf [20]; 
#ifdef RFPLUGIN_DEBUG
        m_Logger->print("RC Transmit");
        m_Logger->println(ms.GetCapture(buf, 0));
#endif
        m_RCSwitch.send(atol(buf), 24);
    }
}

void RFPluginClass::poll()
{
    // todo expire register mode
    if (m_SensorRegisterMode && 
        (m_RegisterModeStart > millis() || millis() - m_RegisterModeStart > 30000))
    {
        exitRegistrationMode();
        m_Logger->println("Sensor registration timeout");
    }

    if (m_RCSwitch.available()) 
    {
        long receivedValue = m_RCSwitch.getReceivedValue();

#ifdef RFPLUGIN_DEBUG
        m_Logger->print("Received "); m_Logger->println(m_RCSwitch.getReceivedValue());
#endif

        if (m_SensorRegisterMode)
        {
            m_SensorRegisterMode = false;
            m_RegisteredSensorId = receivedValue;
        }
        else
        {
            char buf[16];
            ltoa(receivedValue, buf, 10);

            if (m_RegisteredSensors.containsIgnoreCase(buf))
            {
                MQTTHelper.publish((String("rc/") + buf + "/state").c_str(), "on", true);
            }
        }
        
        m_RCSwitch.resetAvailable();
    }
}

RFPluginClass RFPlugin;