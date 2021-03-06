#ifndef RFPLUGIN_H_
#define RFPLUGIN_H_

#include "Arduino.h"
#include <RCSwitch.h>
#include <Regexp.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "MQTTHelper.h"

class RFPluginClass
{
    RCSwitch m_RCSwitch;
    MQTTHelperClass * m_MQTTHelper;
    StringArray m_RegisteredSensors;
    bool m_SensorRegisterMode = false;
    unsigned long m_RegisteredSensorId = 0;
    unsigned long m_RegisterModeStart = 0;
    Stream * m_Logger = NULL;
private:
    static String processor(const String& var);
    String getSensorsJson(RFPluginClass * rfPlugin);
    void enterRegistrationMode();
    void exitRegistrationMode();
public:
    bool begin(MQTTHelperClass * mqttHelper, int receivePin, int sendPin, Stream * logger);
    void bind(AsyncWebServer * server);
    void callback(const char* topic, const char* message);
    void poll();
};

extern RFPluginClass RFPlugin;

#endif