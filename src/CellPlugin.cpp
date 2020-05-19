#include "CellPlugin.h"

String CellPluginClass::processor(const String& var)
{
    if(var == "CallTimeout") return String(CellPlugin.m_CallTimeout);
    if(var == "APN") return CellPlugin.m_APN;
    if(var == "SmtpHost") return CellPlugin.m_SmtpHost;
    if(var == "SmtpPort") return String(CellPlugin.m_SmtpPort);
    if(var == "SmtpUser") return CellPlugin.m_SmtpUser;
    if(var == "SmtpPass") return CellPlugin.m_SmtpPass;

    return String();
}

bool CellPluginClass::begin(Preferences * preferences, MQTTHelperClass * mqttHelper, Stream * module, Stream * debugOut)
{
    m_Preferences = preferences;
    m_MQTTHelper = mqttHelper;

    m_Preferences->begin("cell");
    m_CallTimeout = m_Preferences->getLong("CallTimeout", 15000);
    m_APN = m_Preferences->getString("APN", "internet");
    m_SmtpHost = m_Preferences->getString("SmtpHost", "smtp.gmail.com");
    m_SmtpPort = m_Preferences->getInt("SmtpPort", 465);
    m_SmtpUser = m_Preferences->getString("SmtpUser");
    m_SmtpPass = m_Preferences->getString("SmtpPass");
    m_Preferences->end();
    
    m_Sim.begin(m_APN.c_str(), module, debugOut);

    if (m_Sim.simReset())
    {
        Serial.println("Sim module restarted");
    }

    //m_MQTTHelper->publish("email/error/state", "0");

    m_LastSmsPoll = millis();
    m_LastStatusPoll = millis();
    m_LastSimInitPoll = millis();
    m_LastEmailStatusPoll = millis();

    return true;
}

void CellPluginClass::bind(AsyncWebServer * server)
{
    server->on("/cell/index", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/cell/config.html", "text/html", false, processor);
    });

    server->on("/cell/save", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        CellPlugin.m_CallTimeout = atol(request->getParam("CallTimeout", true, false)->value().c_str());
        CellPlugin.m_APN = request->getParam("APN", true, false)->value();
        CellPlugin.m_SmtpHost = request->getParam("SmtpHost", true, false)->value();
        CellPlugin.m_SmtpPort = atoi(request->getParam("SmtpPort", true, false)->value().c_str());
        CellPlugin.m_SmtpUser = request->getParam("SmtpUser", true, false)->value();
        CellPlugin.m_SmtpPass = request->getParam("SmtpPass", true, false)->value();

        CellPlugin.m_Preferences->begin("cell");
        CellPlugin.m_Preferences->putLong("CallTimeout", CellPlugin.m_CallTimeout);
        CellPlugin.m_Preferences->putString("APN", CellPlugin.m_APN);
        CellPlugin.m_Preferences->putString("SmtpHost", CellPlugin.m_SmtpHost);
        CellPlugin.m_Preferences->putInt("SmtpPort", CellPlugin.m_SmtpPort);
        CellPlugin.m_Preferences->putString("SmtpUser", CellPlugin.m_SmtpUser);
        CellPlugin.m_Preferences->putString("SmtpPass", CellPlugin.m_SmtpPass);
        CellPlugin.m_Preferences->end();
        
        request->redirect("/cell/index");
    });
}

void CellPluginClass::callback(const char* topic, const char* message)
{
    MatchState ms;
    ms.Target((char *)topic);

    if (REGEXP_MATCHED == ms.Match((MQTTHelper.getTopicPrefix() + "/call/(%d+)/command").c_str()))
    {
        char buf [20]; 
        Serial.print("GSM Call:");
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
        Serial.print("SMS Send");
        Serial.println(ms.GetCapture(buf, 0)); // it's a number send SMS to

        if (m_Sim.getActiveCallsCount() > 0)
        {
            m_Sim.hangup();
        }

        m_Sim.sendSms(buf, message);
    }

    if (REGEXP_MATCHED == ms.Match((MQTTHelper.getTopicPrefix() + "/sms/read/command").c_str()))
    {
        // read preconfigured number of SMS and return them as JSON
        IntArray * smsIndexes = m_Sim.getSmsIndexes();

        for(int i = 0; i < smsIndexes->length(); i++)
        {
            DynamicJsonDocument doc(2048);

            int msgSlot = *(smsIndexes->nth(i));
            SmsMessage message = m_Sim.getSmsMessage(msgSlot);

            doc["sender"] = message.sender;
            doc["body"] = message.body;
            
            String resultJson;
            serializeJson(doc, resultJson);

            Serial.println(resultJson + " => " + resultJson.length());
            m_MQTTHelper->publish((String("sms/") + msgSlot + "/read").c_str(), resultJson.c_str(), true);
        }
    }

    if (REGEXP_MATCHED == ms.Match((MQTTHelper.getTopicPrefix() + "/sms/delete/command").c_str()))
    {
        m_Sim.deleteSmsMessage(atoi(message));

    }

    if (REGEXP_MATCHED == ms.Match((MQTTHelper.getTopicPrefix() + "/email/reset/command").c_str()))
    {
        m_Sim.simReset();
        m_SimReady = false;
    }

    if (REGEXP_MATCHED == ms.Match((MQTTHelper.getTopicPrefix() + "/email/send/command").c_str()))
    {
        if (m_Sim.checkRegistration() && m_Sim.checkPacketStatus())
        {
            StaticJsonDocument<256> doc;

            DeserializationError err = deserializeJson(doc, message);

            if (DeserializationError::Ok == err)
            {
                m_EmailInProgress = true;
                m_EmailBusyCount = 0;
                m_LastEmailError = 0;
                m_Sim.sendMail(m_SmtpHost.c_str(), m_SmtpPort,
                    m_SmtpUser.c_str(), m_SmtpPass.c_str(),
                    m_SmtpUser.c_str(), doc["addr"],
                    doc["subj"], doc["body"]);
            }
            else
            {
                Serial.printf("Email MQTT deserialization error %d \r\n", err);
            }
            
        }
    }
}

void CellPluginClass::poll()
{
    if (!m_SimReady && (m_LastSimInitPoll > millis() || millis() - m_LastSimInitPoll > 5000))
    {
        if (!m_Sim.checkRegistration())
        {
            Serial.println("Waiting for SIM registration...");
            m_LastSimInitPoll = millis();
            return;
        }
        else
        {
            if(!m_Sim.checkPacketStatus())
            {
                m_Sim.inetConnect();
            }
            else
            {
                m_Sim.sendDataAndCheckOk("AT+CGPADDR=1");
                m_SimReady = true;
            }
            
        }
        
        m_LastSimInitPoll = millis();
    }


    if (m_EmailInProgress && (m_LastEmailStatusPoll > millis() || millis() - m_LastEmailStatusPoll > 2000))
    {
        int status = m_Sim.checkSmtpProgressStatus();
        switch (status)
        {
        case 0:
            m_EmailInProgress = false;
            break;
        case 1:
            m_EmailBusyCount++;
            if (m_EmailBusyCount > MAX_EMAIL_BUSY_COUNT_TIMEOUT)
            {
                m_LastEmailError = 1;
            }
            break;
        default:
            m_LastEmailError = status;
            break;
        }

        if (m_LastEmailError > 0)
        {
            char buf[5];
            m_MQTTHelper->publish("email/error/state", itoa(m_LastEmailError, buf, 10));
        }

        m_LastEmailStatusPoll = millis();
    }

    // check for SMS messages here
    if (!m_EmailInProgress && (m_LastSmsPoll > millis() || millis() - m_LastSmsPoll > 10000))
    {
        int smsCount = m_Sim.getSmsMessages();
        Serial.println(String("SMS Count:") + smsCount);
        char buf [20]; 
        m_MQTTHelper->publish("sms/incoming/state", itoa(smsCount, buf, 10), true);
        m_LastSmsPoll = millis();
    }

    // hangup call if exist on timeout
    if (!m_EmailInProgress && m_CallInProgress && (m_CallBeginTime > millis() || millis() - m_CallBeginTime > m_CallTimeout))
    {
        if (m_Sim.getActiveCallsCount() > 0)
        {
            m_Sim.hangup();
        }

        m_CallInProgress = false;
    }

    if (!m_EmailInProgress && (m_LastStatusPoll > millis() || millis() - m_LastStatusPoll > 15000))
    {
        if (m_MQTTHelper->isConnected())
        {
            StaticJsonDocument<192> doc;
            doc["ns"] = m_Sim.checkRegistration();
            doc["op"] = m_Sim.getOperatorName();
            doc["sl"] = m_Sim.getSignalLevel();
            doc["pc"] = m_Sim.checkPacketStatus();
            doc["ee"] = m_LastEmailError;
            String status;
            serializeJson(doc, status);
            m_MQTTHelper->publish("sim/poll/state", status.c_str());
        }
        m_LastStatusPoll = millis();
    }
}

CellPluginClass CellPlugin;