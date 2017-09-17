#pragma once
#ifndef __WIN32_SERIAL_H__
#define __WIN32_SERIAL_H__

#include "Platform.h"

class Win32Serial
{
    HANDLE  _hFile;

    void close();

public:
    Win32Serial()
    {
        _hFile = INVALID_HANDLE_VALUE;
    }

    ~Win32Serial()
    {
        close();
    }

    bool init(const char* port);

public:
    bool begin(int baud);
    int  available();
    int  read();
    int  readBytes(uint8_t* buffer, uint_t count);
    int  write(uint8_t data);
    int  write(const uint8_t* data, uint_t count);
    int  write(const char* str);
};


#endif

