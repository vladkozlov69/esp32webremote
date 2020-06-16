#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <Wire.h>
#include <SPI.h>
#include <MDNSHelper.h>
#include "APHelper.h"
#include "OTAHelper.h"
#include "MQTTHelper.h"
#include "RFPlugin.h"

#include <RCSwitch.h>
#include <Regexp.h>
#include <ArduinoJson.h>
#include <StringArray.h>
#include <CellPlugin.h>
#include "LoggerStream.h"


#define RF_TRANSMIT_PIN 12
#define RF_RECEIVE_PIN 27
/*
RF Receive Module data pin	27(26)	-
RF Send Module data pin	-	12
*/

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

Preferences preferences;
MDNSHelper dnsHelper;

LoggerStream LOG(&Serial);

#define SERIAL1_RXPIN 33
#define SERIAL1_TXPIN 32
#define PWRKEY 14

// Replaces placeholder with LED state value
String processor(const String& var)
{
    return String();
}
 
void callback(char* topic, byte* message, unsigned int length) 
{
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++) 
    {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }

    Serial.println();

    RFPlugin.callback(topic, messageTemp.c_str());
    CellPlugin.callback(topic, messageTemp.c_str());
}

void notFound(AsyncWebServerRequest *request) 
{
    request->send(404, "text/plain", "Not found");
}

void doTest();

void setup()
{
    // Serial port for debugging purposes
    Serial.begin(9600);
    Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);

    Serial.println("6 seconds waiting for USB firmware upgrade");
    //delay(6000);

    // Initialize SPIFFS
    if(!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    doTest();

    APHelper.begin(&preferences, &LOG);

    dnsHelper.begin("esp32demo", &LOG);

    MQTTHelper.bind(&server);
    OTAHelper.bind(&server);
    APHelper.bind(&server);
    RFPlugin.bind(&server);
    CellPlugin.bind(&server);

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        AsyncWebServerResponse * response = request->beginResponse(SPIFFS, "/style.css", "text/css");
        response->addHeader("Cache-Control", "max-age=3600");
        request->send(response);
    });

    server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        AsyncWebServerResponse * response = request->beginResponse(SPIFFS, "/jquery.min.js", "text/javascript");
        response->addHeader("Cache-Control", "max-age=3600");
        request->send(response);
    });

    server.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        ESP.restart();
    });

    server.onNotFound(notFound);

    // Start server
    server.begin();

    MQTTHelper.begin(&preferences, &dnsHelper, &LOG, callback);
    RFPlugin.begin(&MQTTHelper, RF_RECEIVE_PIN, RF_TRANSMIT_PIN, &LOG);
    CellPlugin.begin(&preferences, &MQTTHelper, &Serial1, &LOG);
}
 
void loop()
{
    if (OTAHelper.restartRequested()) ESP.restart();
    APHelper.poll();
    MQTTHelper.poll();
    RFPlugin.poll();
    CellPlugin.poll();
}

void doTest()
{
    Sim5360 sim;

    sim.begin("internet.unite.md", &Serial1, &Serial);

    while (!sim.checkRegistration())
    {
        Serial.println("Waiting for SIM registration...");
        delay(2000);
    }

    sim.getNetworkInfo();

    sim.inetConnect();

    while (!sim.checkPacketStatus())
    {
        Serial.println("Waiting for Internet connection...");
        delay(2000);       
    }

    sim.getNetworkInfo();

    sim.postData("www.dweet.io", 443, true, 
    "POST /dweet/for/0123456test HTTP/1.1\r\nContent-type: application/json\r\nHost: dweet.io\r\nContent-Length: 17\r\n\r\n", 
    "{\"d1\":1,\"d2\":\"b\"}");

sim.getNetworkInfo();
    //sim.inetDisconnect();

    while(true) {};
    
}