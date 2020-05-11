#ifndef MDNSHELPER_H_
#define MDNSHELPER_H_

#include <ESPmDNS.h>

class MDNSHelper
{
private:
    /* data */
public:
    bool begin(const char * localHostName);
    String resolve(const char * remoteHostName);
};


#endif