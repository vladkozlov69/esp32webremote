#include "CellPlugin.h"

String CellPluginClass::processor(const String& var)
{
    // todo WEB config stuff
}

bool CellPluginClass::begin(Preferences * preferences, MQTTHelperClass * mqttHelper, Stream * module, Stream * debugOut)
{
    m_Preferences = preferences;
    m_MQTTHelper = mqttHelper;

    m_Preferences->begin("cell");
    m_CallTimeout = m_Preferences->getLong("CallTimeout", 15000);
    m_APN = m_Preferences->getString("APN", "");
    m_Preferences->end();
    
    m_Sim.begin(m_APN.c_str(), module, debugOut);
}

void CellPluginClass::bind(AsyncWebServer * server)
{
    // todo WEB config stuff
}

void CellPluginClass::callback(char* topic, byte* message, unsigned int length)
{
    MatchState ms;
    ms.Target(topic);

    if (REGEXP_MATCHED == ms.Match((MQTTHelper.getTopicPrefix() + "/call/(%d+)/command").c_str()))
    {
        char buf [20]; 
        Serial.print("GSM Call");
        Serial.println(ms.GetCapture(buf, 0)); // it's a number to call
        
        if (m_Sim.getActiveCallsCount() > 0)
        {
            m_Sim.hangup();
        }

        m_Sim.dial(buf);
        m_CallInProgress = true;
        m_CallBeginTime = millis();
    }

    if (REGEXP_MATCHED == ms.Match((MQTTHelper.getTopicPrefix() + "/sms/(%d+)/command").c_str()))
    {
        char buf [20]; 
        char msg [200];
        Serial.print("SMS Send");
        Serial.println(ms.GetCapture(buf, 0)); // it's a number send SMS to

        strncpy((char *)message, msg, length);
        m_Sim.sendSms(buf, (const char *)msg);
        if (m_CallInProgress && (m_CallBeginTime > millis() || millis() - m_CallBeginTime > m_CallTimeout))
        {
            if (m_Sim.getActiveCallsCount() > 0)
            {
                m_Sim.hangup();
            }

            m_CallInProgress = false;
        }

        m_Sim.sendSms(buf, "sms test");
    }
}

void CellPluginClass::poll()
{
    // check for SMS messages here

    // hangup call if exist on timeout
    if (m_Sim.getActiveCallsCount() > 0)
    {

    }
}

CellPluginClass CellPlugin;