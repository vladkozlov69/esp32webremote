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
private:
    static String processor(const String& var);
    String getSensorsJson(RFPluginClass * rfPlugin);
    void enterRegistrationMode();
    void exitRegistrationMode();
public:
    bool begin(MQTTHelperClass * mqttHelper, int receivePin, int sendPin);
    void bind(AsyncWebServer * server);
    void callback(char* topic, byte* message, unsigned int length);
    void poll();
};

extern RFPluginClass RFPlugin;

#endif