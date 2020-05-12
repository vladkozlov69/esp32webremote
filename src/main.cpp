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
#include <ArduinoJson.h>
#include <StringArray.h>

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

bool sensorRegisterMode = false;
long registeredSensorId = 0;
StringArray acls;
String listjson;

// Replaces placeholder with LED state value
String processor(const String& var)
{
    if(var == "STATE") return digitalRead(ledPin) ? "ON" : "OFF";
    if(var == "sensorId") return String(registeredSensorId);
    if(var == "waiting") return sensorRegisterMode ? "false" : "true";
    if(var == "listjson") return listjson;

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

    server.on("/rf/index", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/rf/index.html", String(), false, processor);
    });

    server.on("/rf/list.json", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        DynamicJsonDocument doc(4096);

        JsonArray acl = doc.createNestedArray("acl");

        for(int i = 0; i < acls.length(); i++)
        {
            String e = *(acls.nth(i));
            Serial.println(e);
            acl.add(e);
        }

        listjson = "";
        serializeJson(doc, listjson), 

        Serial.println(listjson);

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", listjson);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    server.on("/rf/save", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        // todo save json to SPIFFS
        request->redirect("/rf/index");
    });

    server.on("/rf/remove", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        // todo remove by id param
        request->redirect("/rf/index");
    });

    server.on("/rf/register", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        sensorRegisterMode = true;

        request->send(SPIFFS, "/rf/register.html", String(), false, processor);
    });

    server.on("/rf/cancel", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        sensorRegisterMode = false;
        registeredSensorId = 0;

        request->redirect("/rf/index");
    });

    server.on("/rf/confirm", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        sensorRegisterMode = false;

        request->send(SPIFFS, "/rf/confirm.html", String(), false, processor);
    });

    server.on("/rf/state.json", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/rf/state.json", String(), false, processor);
    });

    // Start server
    server.begin();

    MQTTHelper.begin(&preferences, &dnsHelper, callback);

    // load rf ACL
    File file = SPIFFS.open("/config/rf.json", "r");
    if (!file)
    {
        Serial.println("No RF ACL defined");
    } 
    else 
    {
        size_t size = file.size();

        DynamicJsonDocument doc(4096);

        DeserializationError result = deserializeJson(doc, file);


        if (result.code() == DeserializationError::Ok)
        {
            JsonArray acl = doc["acl"];

            for (int i = 0; i < acl.size(); i++)
            {
                String elem = acl.getElement(i).as<char*>();
                Serial.println(elem);
                acls.add(elem);
            }
        }

        file.close();

        for(int i = 0; i < acls.length(); i++)
        {
            Serial.println(*(acls.nth(i)));
        }
    }
}
 
void loop()
{
    MQTTHelper.poll();

    if (OTAHelper.restartRequested()) ESP.restart();

    if (rcTransmitter.available()) 
    {
        Serial.print("Received "); Serial.println( rcTransmitter.getReceivedValue() );

        //MQTTHelper.publish((String("rc/") + rcTransmitter.getReceivedValue() + "/state").c_str(), "on", true);

        if (sensorRegisterMode)
        {
            sensorRegisterMode = false;
            registeredSensorId = rcTransmitter.getReceivedValue();
        }

        rcTransmitter.resetAvailable();
    }
}