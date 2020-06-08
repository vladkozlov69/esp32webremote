#include "MDNSHelper.h"


bool MDNSHelper::begin(const char * localHostName, Stream * logger)
{
    m_Logger = logger;

    if (!MDNS.begin(localHostName)) 
    {
#ifdef MDNSHELPER_DEBUG
        m_Logger->println("Error setting up MDNS responder!");
#endif
        return false;
    } 
#ifdef MDNSHELPER_DEBUG
    else 
    {
        m_Logger->println("Finished intitializing the MDNS client...");
    }

    m_Logger->println("mDNS responder started");
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
        m_Logger->println("Trying again to resolve mDNS");
#endif
        delay(250);
        counter--;
        serverIp = MDNS.queryHost(remoteHostName);
    }

#ifdef MDNSHELPER_DEBUG
    m_Logger->print("Resolved:");
    m_Logger->print(remoteHostName);
    m_Logger->print(" => ");
    m_Logger->println(serverIp.toString());
#endif

    return serverIp.toString();
}