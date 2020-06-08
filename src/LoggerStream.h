#ifndef LOGGERSTREAM_H_
#define LOGGERSTREAM_H_

#include "Stream.h"

class LoggerStream : public Stream
{
private:
    Stream * m_WrappedStream;
    bool enabled;
public:
    LoggerStream(Stream * wrappedStream);
    void setEnabled(bool enable);
    virtual size_t write(uint8_t ch);
    virtual size_t write(const uint8_t *buffer, size_t size);
    int available(void);
    int peek(void);
    int read(void);
    void flush(void);
};

#endif