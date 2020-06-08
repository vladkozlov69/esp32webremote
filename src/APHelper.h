#ifndef APHELPER_H_
#define APHELPER_H_

#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <Preferences.h>
#include "CaptiveRequestHandler.h"

class APHelperClass
{
    Preferences * m_Preferences;
    String m_APHost;
    String m_APPass;
    bool m_CaptivePortalPassed = false;
    DNSServer * m_DnsServer = NULL;
    Stream * m_Logger = NULL;
private:
    static String processor(const String& var);
public:
    void begin(Preferences * preferences, Stream * logger);
    void bind(AsyncWebServer * server);
    static void factoryReset(APHelperClass * apHelper);
    void poll();
};

extern APHelperClass APHelper;

#endif