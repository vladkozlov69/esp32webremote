#include "MDNSHelper.h"


bool MDNSHelper::begin(const char * localHostName)
{
    if (!MDNS.begin(localHostName)) 
    {
#ifdef MDNSHELPER_DEBUG
        Serial.println("Error setting up MDNS responder!");
#endif
        return false;
    } 
#ifdef MDNSHELPER_DEBUG
    else 
    {
        Serial.println("Finished intitializing the MDNS client...");
    }

    Serial.println("mDNS responder started");
#endif

    return true;
}

String MDNSHelper::resolve(const char * remoteHostName)
{
    int counter = 3;
    IPAddress serverIp = MDNS.queryHost(remoteHostName);
    while (serverIp.toString() == "0.0.0.0" && counter > 0) 
    {
#ifdef MDNSHELPER_DEBUG
        Serial.println("Trying again to resolve mDNS");
#endif
        delay(250);
        counter--;
        serverIp = MDNS.queryHost(remoteHostName);
    }

#ifdef MDNSHELPER_DEBUG
    Serial.print("Resolved:");
    Serial.print(remoteHostName);
    Serial.print(" => ");
    Serial.println(serverIp.toString());
#endif

    return serverIp.toString();
}