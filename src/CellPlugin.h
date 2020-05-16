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
    unsigned long m_LastSmsPoll;
    unsigned long m_LastStatusPoll;
    bool m_EmailInProgress = false;
    bool m_SimReady = false;
    String m_SmtpHost;
    int m_SmtpPort;
    String m_SmtpUser;
    String m_SmtpPass;
private:
    static String processor(const String& var);
public:
    bool begin(Preferences * preferences, MQTTHelperClass * mqttHelper, Stream * module, Stream * debugOut);
    void bind(AsyncWebServer * server);
    void callback(const char* topic, const char* message);
    void poll();
};

extern CellPluginClass CellPlugin;

#endif