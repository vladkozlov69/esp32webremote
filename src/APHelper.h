#ifndef APHELPER_H_
#define APHELPER_H_

#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <Preferences.h>

class APHelperClass
{
    Preferences * m_Preferences;
    String m_APHost;
    String m_APPass;
private:
    static String processor(const String& var);
public:
    void begin(Preferences * preferences);
    void bind(AsyncWebServer * server);
    static void factoryReset(APHelperClass * apHelper);
};

extern APHelperClass APHelper;

#endif