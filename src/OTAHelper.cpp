#include "OTAHelper.h"


void OTAHelperClass::bind(AsyncWebServer * server)
{
    if (SPIFFS.begin(false))
    {
        server->on("/ota", HTTP_GET, [](AsyncWebServerRequest *request)
        {
            request->send(SPIFFS, "/ota/ota.html", "text/html", false, OTAHelper.processor);
        });

        // handler for the /update form POST (once file upload finishes)
        server->on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
        {
            request->send(200);
        }, handle_update_progress_cb);
    }
}

String OTAHelperClass::processor(const String& var)
{
    return String();
}

void OTAHelperClass::handle_update_progress_cb(AsyncWebServerRequest *request, String filename, size_t index, 
    uint8_t *data, size_t len, bool final)
{
    bool isSpiffs = filename.startsWith("spiffs");

    uint32_t free_space = isSpiffs 
        ? getSpiffsPartitionSize() 
        : (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;

    if (!index)
    {
#ifdef OTAHELPER_DEBUG
        Serial.print("Update:");
        Serial.println(filename);
#endif
        //Update.runAsync(true);
        if (!Update.begin(free_space, isSpiffs ? U_SPIFFS : U_FLASH)) 
        {
            Update.printError(Serial);
        }
    }

#ifdef OTAHELPER_DEBUG
    Serial.print("..writing..");
    Serial.print(len);
    Serial.print("..remaining..");
    Serial.println(Update.remaining());
#endif

    if (Update.write(data, len) != len) 
    {
        Update.printError(Serial);
    }

    if (final) 
    {
        if (!Update.end(true))
        {
            Update.printError(Serial);
        } 
        else 
        {
#ifdef OTAHELPER_DEBUG
            Serial.println("Update complete");
#endif
            m_Restart = true;
        }
    }
}

bool OTAHelperClass::m_Restart = false;

uint32_t OTAHelperClass::getSpiffsPartitionSize()
{
    const esp_partition_t *spiffs_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);

    return (NULL != spiffs_partition) ? spiffs_partition->size : 0;
}

OTAHelperClass OTAHelper;