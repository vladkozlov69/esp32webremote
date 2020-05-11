#ifndef OTAHELPER_H_
#define OTAHELPER_H_

#include "ESPAsyncWebServer.h"
#include <Update.h>
#include "SPIFFS.h"

class OTAHelperClass
{
    static bool m_Restart;
private:
    static void handle_update_progress_cb(AsyncWebServerRequest *request, 
        String filename, size_t index, 
        uint8_t *data, size_t len, bool final);
    static uint32_t getSpiffsPartitionSize();
    static String processor(const String& var);
public:
    void bind(AsyncWebServer * server);
    bool restartRequested() { return m_Restart; }
};

extern OTAHelperClass OTAHelper;


#endif