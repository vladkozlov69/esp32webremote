#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <Wire.h>
#include <SPI.h>
#include <MDNSHelper.h>
#include "APHelper.h"
#include "OTAHelper.h"
#include "MQTTHelper.h"

#include <RCSwitch.h>
#include <Regexp.h>

RCSwitch rcTransmitter = RCSwitch();

#define RF_TRANSMIT_PIN 12
#define RF_RECEIVE_PIN 27
/*
RF Receive Module data pin	27(26)	-
RF Send Module data pin	-	12
*/

// Set LED GPIO
const int ledPin = 2;
// Stores LED state
String ledState;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

Preferences preferences;
MDNSHelper dnsHelper;

// Replaces placeholder with LED state value
String processor(const String& var)
{
    if(var == "STATE") return digitalRead(ledPin) ? "ON" : "OFF";

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

    // Feel free to add more if statements to control more GPIOs with MQTT

    // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
    // Changes the output state according to the message
    if (String(topic) == "esp32/command") 
    {
        Serial.print("Changing output to ");
        if(messageTemp == "on")
        {
            Serial.println("on");
            digitalWrite(ledPin, HIGH);
        }
        else if(messageTemp == "off")
        {
            Serial.println("off");
            digitalWrite(ledPin, LOW);
        }
    }

    MatchState ms;
    ms.Target(topic);

    if (REGEXP_MATCHED == ms.Match((MQTTHelper.getTopicPrefix() + "/rc/(%d+)/command").c_str()))
    {
        char buf [20]; 
        Serial.print("RC Transmit");
        Serial.println(ms.GetCapture(buf, 0));
        rcTransmitter.send(atol(buf), 24);
    }
}

void setup()
{
    // Serial port for debugging purposes
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);

    rcTransmitter.enableTransmit(RF_TRANSMIT_PIN);
    rcTransmitter.enableReceive(RF_RECEIVE_PIN);

    // Initialize SPIFFS
    if(!SPIFFS.begin(false))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    APHelper.begin(&preferences);

    dnsHelper.begin("esp32demo");

    MQTTHelper.bind(&server);
    OTAHelper.bind(&server);
    APHelper.bind(&server);

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    // Route to set GPIO to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        digitalWrite(ledPin, HIGH);   
        MQTTHelper.publish("state", "1"); 
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    // Route to set GPIO to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        digitalWrite(ledPin, LOW);  
        MQTTHelper.publish("state", "0"); 
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/style.css", "text/css");
    });

    server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/jquery.min.js", "text/javascript");
    });

    server.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        ESP.restart();
    });

    // Start server
    server.begin();

    MQTTHelper.begin(&preferences, &dnsHelper, callback);
}
 
void loop()
{
    MQTTHelper.poll();

    if (OTAHelper.restartRequested()) ESP.restart();

    if (rcTransmitter.available()) 
    {
        Serial.print("Received "); Serial.println( rcTransmitter.getReceivedValue() );

        MQTTHelper.publish((String("rc/") + rcTransmitter.getReceivedValue() + "/state").c_str(), "on", true);

        rcTransmitter.resetAvailable();
    }
}