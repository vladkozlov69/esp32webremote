#include "APHelper.h"

String APHelperClass::processor(const String& var)
{
    if(var == "hostname") return APHelper.m_APHost;
    if(var == "password") return APHelper.m_APPass;

    return String();
}

void APHelperClass::begin(Preferences * preferences)
{
    m_Preferences = preferences;

    m_Preferences->begin("ap", false);
    m_APHost = m_Preferences->getString("AP_Host");
    m_APPass = m_Preferences->getString("AP_Pass");
    m_Preferences->end();

    // Now, start WiFi AP
    if (m_APHost.length() == 0)
    {
        WiFi.softAP("esp32", "esp32admin");
        Serial.print("IP address: ");
        Serial.println(WiFi.softAPIP());
        m_DnsServer = new DNSServer(); 
        m_DnsServer->start(53, "*", WiFi.softAPIP());
    }
    else
    {
        // Connect to Wi-Fi
        WiFi.begin(m_APHost.c_str(), m_APPass.c_str());
        while (WiFi.status() != WL_CONNECTED) 
        {
            delay(1000);
            Serial.println("Connecting to WiFi..");
        }
        // Print ESP32 Local IP Address
        Serial.println(WiFi.localIP());
    }
}

bool ON_AP_FILTER1(AsyncWebServerRequest *request) {
  return WiFi.softAPIP() != request->client()->localIP();
}

void APHelperClass::bind(AsyncWebServer * server)
{
    if (m_APHost.length() == 0)
    {
        Serial.println("Added CaptiveRequestHandler");
        server->addHandler(new CaptiveRequestHandler()).setFilter({ON_AP_FILTER});//only when requested from AP
    }

    server->on("/ap/config", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/ap/config.html", String(), false, processor);
    });

    server->on("/ap/factory", HTTP_POST, [](AsyncWebServerRequest *request)
    {  
        factoryReset(&APHelper);

        request->redirect("/ap/config");
    });

    server->on("/ap/save", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        APHelper.m_APHost = request->getParam("hostname", true, false)->value();
        APHelper.m_APPass = request->getParam("password", true, false)->value();

        APHelper.m_Preferences->begin("ap", false);
        APHelper.m_Preferences->putString("AP_Host", APHelper.m_APHost);
        APHelper.m_Preferences->putString("AP_Pass", APHelper.m_APPass);
        APHelper.m_Preferences->end();

        request->redirect("/ap/config");
    });
}

void APHelperClass::factoryReset(APHelperClass * apHelper)
{
    apHelper->m_APHost = "";
    apHelper->m_APPass = "";

    apHelper->m_Preferences->begin("ap", false);
    apHelper->m_Preferences->putString("APHost", apHelper->m_APHost);
    apHelper->m_Preferences->putString("APPass", apHelper->m_APPass);
    apHelper->m_Preferences->end();
}

void APHelperClass::poll()
{
    if (m_DnsServer)
    {
        m_DnsServer->processNextRequest();
    }
}

APHelperClass APHelper;