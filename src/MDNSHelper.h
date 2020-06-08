#ifndef MDNSHELPER_H_
#define MDNSHELPER_H_

#include <ESPmDNS.h>

class MDNSHelper
{
private:
    Stream * m_Logger = NULL;
public:
    bool begin(const char * localHostName, Stream * logger);
    String resolve(const char * remoteHostName);
};


#endif