#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <PubSubClient.h>
#include <Wire.h>
#include <SPI.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Preferences.h>


const char* AP_SSID = "esp32demo";
const char* AP_PASSWORD = "esp32demo";
const char* MQTT_HOST = "hassistant";

String mqttHost = String();
String apHost = String();
String apPass = String();
String localMacAddr;

// Set LED GPIO
const int ledPin = 2;
// Stores LED state
String ledState;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

WiFiClient espClient;

PubSubClient mqttClient(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

Preferences preferences;

// on my laptop (Ubuntu) the equivalent command is: `avahi-resolve-host-name -4 mqtt-broker.local`
String findMDNS(String mDnsHost) 
{ 
    IPAddress serverIp = MDNS.queryHost(mDnsHost);
    while (serverIp.toString() == "0.0.0.0") 
    {
        Serial.println("Trying again to resolve mDNS");
        delay(250);
        serverIp = MDNS.queryHost(mDnsHost);
    }

    return serverIp.toString();
}

// Replaces placeholder with LED state value
String processor(const String& var)
{
    if(var == "STATE") return digitalRead(ledPin) ? "ON" : "OFF";
    if(var == "hostname") return apHost;
    if(var == "password") return apPass;
    if(var == "mqtthost") return mqttHost;

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
}

void reconnect() 
{
    // Loop until we're reconnected
    while (!mqttClient.connected()) 
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (mqttClient.connect("ESP8266Client")) 
        {
            Serial.println("connected");
            // Subscribe
            mqttClient.subscribe("esp32/command");
        } 
        else 
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

uint32_t getSpiffsPartitionSize()
{
 	const esp_partition_t *spiffs_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);

    return spiffs_partition->size;
}

static void handle_update_progress_cb(AsyncWebServerRequest *request, String filename, size_t index, 
    uint8_t *data, size_t len, bool final) 
{
    bool isSpiffs = filename.startsWith("spiffs");

    uint32_t free_space = isSpiffs 
        ? getSpiffsPartitionSize() 
        : (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;

    if (!index)
    {
        Serial.print("Update:");
        Serial.println(filename);
        //Update.runAsync(true);
        if (!Update.begin(free_space, isSpiffs ? U_SPIFFS : U_FLASH)) 
        {
            Update.printError(Serial);
        }
    }

    Serial.print("..writing..");
    Serial.print(len);
    Serial.print("..remaining..");
    Serial.println(Update.remaining());

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
            //restartNow = true;//Set flag so main loop can issue restart call
            Serial.println("Update complete");
        }
    }
}

void setup()
{
    // Serial port for debugging purposes
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);

    localMacAddr = WiFi.macAddress();
    localMacAddr.replace(":","");
    Serial.print("Local MAC:");
    Serial.println(localMacAddr);

    preferences.begin("esp32demo", false);
    mqttHost = preferences.getString("mqttServer", MQTT_HOST);
    apHost = preferences.getString("AP_Host");
    apPass = preferences.getString("AP_Pass");
    preferences.end();

    // Initialize SPIFFS
    if(!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // Now, start WiFi AP or connect
    if (apHost.length() == 0)
    {
        WiFi.softAP(AP_SSID, AP_PASSWORD);
        Serial.println();
        Serial.print("IP address: ");
        Serial.println(WiFi.softAPIP());
    }
    else
    {
        // Connect to Wi-Fi
        WiFi.begin(apHost.c_str(), apPass.c_str());
        while (WiFi.status() != WL_CONNECTED) 
        {
            delay(1000);
            Serial.println("Connecting to WiFi..");
        }
        // Print ESP32 Local IP Address
        Serial.println(WiFi.localIP());
    }

    // Start mDNS responder
    if (!MDNS.begin("esp32demo")) 
    {
        Serial.println("Error setting up MDNS responder!");
    } 
    else 
    {
        Serial.println("Finished intitializing the MDNS client...");
    }

    Serial.println("mDNS responder started");

    Serial.print("Resolving:");
    Serial.print(mqttHost.c_str());
    String mqtt_ip = findMDNS(mqttHost);
    Serial.print(" => ");
    Serial.println(mqtt_ip);

    mqttClient.setServer(mqtt_ip.c_str(), 1883);
    mqttClient.setCallback(callback);

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.on("/ota.html", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/ota.html", String(), false, processor);
    });

    // Route to load style.css file
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/style.css", "text/css");
    });

    // Route to load jquery.min.js file
    server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/jquery.min.js", "text/javascript");
    });

    // Route to set GPIO to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        digitalWrite(ledPin, HIGH);   
        mqttClient.publish((String() + "esp32/" + localMacAddr +"/state").c_str(), "1"); 
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    // Route to set GPIO to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        digitalWrite(ledPin, LOW);  
        mqttClient.publish((String() + "esp32/" + localMacAddr +"/state").c_str(), "0");   
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.on("/config.html", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/config.html", String(), false, processor);
    });

    server.on("/factory", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        digitalWrite(ledPin, LOW);  
        mqttClient.publish("esp32/state", "0");   
        preferences.begin("esp32demo", false);
        preferences.putString("mqttServer", MQTT_HOST);
        preferences.putString("AP_Host", "");
        preferences.putString("AP_Pass", "");
        preferences.end();
        request->send(SPIFFS, "/reboot.html", String(), false, processor);
        ESP.restart();
    });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        apHost = request->getParam("hostname", true, false)->value();
        apPass = request->getParam("password", true, false)->value();
        mqttHost = request->getParam("mqtthost", true, false)->value();

        preferences.begin("esp32demo", false);
        preferences.putString("mqttServer", mqttHost);
        preferences.putString("AP_Host", apHost);
        preferences.putString("AP_Pass", apPass);
        preferences.end();
        request->send(SPIFFS, "/config.html", String(), false, processor);
    });

    // handler for the /update form POST (once file upload finishes)
    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        request->send(200);
    }, handle_update_progress_cb);

    server.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        ESP.restart();
    });

    // Start server
    server.begin();
}
 
void loop()
{
    if (!mqttClient.connected()) 
    {
        reconnect();
    }

    mqttClient.loop();
}