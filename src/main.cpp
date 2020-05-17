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

#define SERIAL1_RXPIN 33
#define SERIAL1_TXPIN 32
#define PWRKEY 14

unsigned long simLastPoll;
Sim5360 sim;

int pwrOff = 15000;
int pwrOn = 30000;

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

    RFPlugin.callback(topic, messageTemp.c_str());
    CellPlugin.callback(topic, messageTemp.c_str());
}

void setup()
{
    delay(6000);
    // Serial port for debugging purposes
    Serial.begin(9600);
    Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);

    pinMode(ledPin, OUTPUT);

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
    RFPlugin.bind(&server);
    CellPlugin.bind(&server);

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

    RFPlugin.begin(&MQTTHelper, RF_RECEIVE_PIN, RF_TRANSMIT_PIN);

    CellPlugin.begin(&preferences, &MQTTHelper, &Serial1, &Serial);
    // pinMode(PWRKEY, OUTPUT);
    // digitalWrite(PWRKEY, HIGH);
  
    

    // sim.begin("", &Serial1, NULL);

    // simLastPoll = millis();
}
 
void loop()
{
    if (OTAHelper.restartRequested()) ESP.restart();
    MQTTHelper.poll();
    RFPlugin.poll();
    CellPlugin.poll();

/*
    if (millis() > pwrOff && pwrOff > 0)
    {
        pwrOff = 0;
        Serial.println("Turning off");
        digitalWrite(PWRKEY, LOW);
        delay(2000);
        digitalWrite(PWRKEY, HIGH);
    }

    if (millis() > pwrOn && pwrOn > 0)
    {
        Serial.println("Turning on");
        pwrOn = 0;
        digitalWrite(PWRKEY, LOW);
        delay(250);
        digitalWrite(PWRKEY, HIGH);
    }
*/
/*
    if (millis() - simLastPoll > 2000)
    {
        Serial.print("GSM Network status:"); Serial.println(sim.checkRegistration());    
        Serial.print("GSM operator:"); Serial.println(sim.getOperatorName());
        Serial.print("GSM signal level:"); Serial.println(sim.getSignalLevel());
        Serial.print("Packet connection status:"); Serial.println(sim.checkPacketStatus());
        Serial.print("Active calls:"); Serial.println(sim.getActiveCallsCount());
        Serial.print("SMS messages:"); Serial.println(sim.getSmsMessages());

        // LinkedList<int> * smsIndexes = sim.getSmsIndexes();

        // for (int i = 0; i < smsIndexes->size(); i++)
        // {
        // SmsMessage msg = sim.getSmsMessage((*smsIndexes)[i]);
        // Serial.print(msg.sender);
        // Serial.print(" => ");
        // Serial.println(msg.body);
        // }

    //    Serial.println(sim.sendData("AT+CMGF=1", 2000));
    //    Serial.println(sim.sendData("AT+CMGL=\"ALL\"", 2000));
    //    Serial.println(sim.sendData("AT+CMGR=0", 2000));
    //    Serial.println(sim.sendData("AT+CMGD=0", 2000));
        simLastPoll = millis();
    }
*/
}