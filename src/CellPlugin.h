#ifndef CELLPLUGIN_H_
#define CELLPLUGIN_H_

#include "Arduino.h"
#include <sim5360.h>
#include <Regexp.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "MQTTHelper.h"

class CellPluginClass
{
    Preferences * m_Preferences;
    Sim5360 m_Sim;
    MQTTHelperClass * m_MQTTHelper;
    String m_APN;
    unsigned long m_CallBeginTime = 0;
    bool m_CallInProgress = false;
    unsigned long m_CallTimeout;
private:
    static String processor(const String& var);
public:
    bool begin(Preferences * preferences, MQTTHelperClass * mqttHelper, Stream * module, Stream * debugOut);
    void bind(AsyncWebServer * server);
    void callback(char* topic, byte* message, unsigned int length);
    void poll();
};

extern CellPluginClass CellPlugin;

#endif