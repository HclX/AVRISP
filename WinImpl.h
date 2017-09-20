#pragma once
#ifndef __WIN32_IMPL_H__
#define __WIN32_IMPL_H__

#include <Windows.h>
#include <assert.h>
#include <vector>
#include <stdio.h>

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned int    uint_t;

template <int portId>
class Win32Serial
{
    HANDLE  _hFile;

    void close()
    {
        if (_hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(_hFile);
            _hFile = INVALID_HANDLE_VALUE;
        }
    }

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
    bool begin(int baudrate)
    {
        if (_hFile == INVALID_HANDLE_VALUE)
        {
            char port[16];
            snprintf(port, sizeof(port), "COM%d", portId);

            _hFile = CreateFileA(port, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
            if (_hFile == INVALID_HANDLE_VALUE)
            {
                return false;
            }
        }

        DCB dcb = { 0 };
        if (!BuildCommDCBA("baud=115200 parity=N data=8 stop=1", &dcb))
        {
            return false;
        }

        dcb.BaudRate = baudrate;
        if (!SetCommState(_hFile, &dcb))
        {
            return false;
        }

        return true;
    }

    int  available()
    {
        assert(_hFile != INVALID_HANDLE_VALUE);

        COMSTAT stat = { 0 };
        DWORD dwError = 0;
        if (!ClearCommError(_hFile, &dwError, &stat))
        {
            return -1;
        }

        return stat.cbInQue;
    }

    int  read(uint8_t* buffer, uint_t count)
    {
        assert(_hFile != INVALID_HANDLE_VALUE);

        UINT8 ch;
        DWORD dwRead = 0;
        if (ReadFile(_hFile, buffer, count, &dwRead, NULL))
        {
            assert(dwRead == count);
            return count;
        }

        printf("ReadFile failed, GLE=%d\n", GetLastError());
        return -1;
    }

    int  read()
    {
        uint8_t ch;
        return read(&ch, 1) == 1 ? ch : -1;
    }

    int  write(const uint8_t* data, uint_t count)
    {
        assert(_hFile != INVALID_HANDLE_VALUE);
        assert(data != NULL);

        DWORD dwWritten = 0;
        if (WriteFile(_hFile, data, count, &dwWritten, NULL))
        {
            assert(dwWritten == count);
            return dwWritten;
        }

        printf("WriteFile failed, GLE=%d\n", GetLastError());
        return -1;
    }

    int  write(uint8_t data)
    {
        return write(&data, 1);
    }

    int  write(const char* str)
    {
        if (str == NULL)
        {
            return 0;
        }
        else
        {
            return write((const uint8_t*)str, strlen(str));
        }
    }

    void discard()
    {
        uint8_t buf[256];
        for (;;)
        {
            int n = available();
            if (n <= 0)
                break;

            if (n > sizeof(buf))
            {
                n = sizeof(buf);
            }

            read(buf, n);
        }
    }
};

struct Sys
{
public:
    void delay(int ms)
    {
        Sleep(ms);
    }

    void printf(const char* fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }

    uint32_t millis()
    {
        return GetTickCount();
    }
};

class SimAVR
{
    std::vector<uint16_t>   _flash;
    std::vector<uint16_t>   _page;

    std::vector<uint8_t>    _eeprom;

    std::vector<uint8_t>    _params;
    const char*             _sig;
    uint8_t                 _fuses[3];
    uint8_t                 _lock;
    uint8_t                 _cal;
    bool                    _reset;
    bool                    _pg_en;
public:
    SimAVR()
        : _params(20)
        , _reset(false)
        , _pg_en(false)
    {
    }

    ~SimAVR()
    {
    }

    bool    setDevice(const uint8_t* params);
    bool    setDeviceExt(const uint8_t* params);

    bool    start();
    void    finish();
    uint8_t xfer(uint8_t b1, uint16_t b2b3 = 0, uint8_t b4 = 0);
    uint8_t xfer(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
};

#endif

