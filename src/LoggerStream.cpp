#include "LoggerStream.h"

LoggerStream::LoggerStream(Stream * wrappedStream)
{
    m_WrappedStream = wrappedStream;
    enabled = (NULL != m_WrappedStream);
}

void LoggerStream::setEnabled(bool enable)
{
    enabled = (NULL != m_WrappedStream) && enable;
}

size_t LoggerStream::write(uint8_t ch)
{
    if (enabled) 
    {
        return m_WrappedStream->write(ch);
    }

    return 0;
}

size_t LoggerStream::write(const uint8_t *buffer, size_t size)
{
    if (enabled) 
    {
        return m_WrappedStream->write(buffer, size);
    }
    
    return 0;
}

int LoggerStream::available(void)
{
    return 0;
}

int LoggerStream::peek(void)
{
    return 0;
}

int LoggerStream::read(void)
{
    return 0;
}

void LoggerStream::flush(void)
{
    if (enabled)
    {
        m_WrappedStream->flush();
    }
}