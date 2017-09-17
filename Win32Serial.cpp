#include "stdafx.h"
#include "Win32Serial.h"

void Win32Serial::close()
{
    if (_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hFile);
        _hFile = INVALID_HANDLE_VALUE;
    }
}

bool Win32Serial::init(const char* port)
{
    close();

    _hFile = CreateFileA(port, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (_hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    return true;
}

bool Win32Serial::begin(int baudrate)
{
    ASSERT(_hFile != INVALID_HANDLE_VALUE);

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

int Win32Serial::available()
{
    ASSERT(_hFile != INVALID_HANDLE_VALUE);

    COMSTAT stat = { 0 };
    DWORD dwError = 0;
    if (!ClearCommError(_hFile, &dwError, &stat))
    {
        return -1;
    }

    return stat.cbInQue;
}
int Win32Serial::readBytes(uint8_t* buffer, uint_t count)
{
    ASSERT(_hFile != INVALID_HANDLE_VALUE);

    UINT8 ch;
    DWORD dwRead = 0;
    if (ReadFile(_hFile, buffer, count, &dwRead, NULL))
    {
        ASSERT(dwRead == count);
        return count;
    }

    DbgPrintf("ReadFile failed, GLE=%d\n", GetLastError());
    return -1;
}

int Win32Serial::read()
{
    uint8_t ch;
    return readBytes(&ch, 1) == 1 ? ch : -1;
}

int Win32Serial::write(const uint8_t* data, uint_t size)
{
    ASSERT(_hFile != INVALID_HANDLE_VALUE);
    ASSERT(data != NULL);

    DWORD dwWritten = 0;
    if (WriteFile(_hFile, data, size, &dwWritten, NULL))
    {
        ASSERT(dwWritten == size);
        return dwWritten;
    }

    DbgPrintf("WriteFile failed, GLE=%d\n", GetLastError());
    return -1;
}

int Win32Serial::write(uint8_t val)
{
    return write(&val, 1);
}

int Win32Serial::write(const char* str)
{
    if (str == NULL)
    {
        return 0;
    }

    return write((const uint8_t*)str, strlen(str));
}

Win32Serial Serial;
