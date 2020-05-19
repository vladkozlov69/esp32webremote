#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"

class CaptiveRequestHandler : public AsyncWebHandler {

public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request)
    {
        return (request->header("Cookie").indexOf("cp=1") == -1);
    }

    void handleRequest(AsyncWebServerRequest *request) 
    {
        Serial.println("CaptiveRequestHandler");
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        response->print("<!DOCTYPE html><html><head><title>Captive Portal</title>");
        response->print("<meta http-equiv=\"refresh\" content=\"0;/\"></head><body>");
        response->print("<p>This is out captive portal front page.</p>");
        response->printf("<p>You were trying to reach: http://%s</p>", request->host().c_str());
        response->printf("<p>Try opening <a href='http://%s'>%s</a> instead</p>", WiFi.softAPIP().toString().c_str(), WiFi.softAPIP().toString().c_str());
        response->print("</body></html>");
        response->addHeader("Set-Cookie", "cp=1");
        request->send(response);
    }
};
