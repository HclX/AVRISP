#ifndef __ARDUINO_IMPL_H__
#define __ARDUINO_IMPL_H__

#include <Arduino.h>

struct SerialPort
{
    bool begin(int baudrate)
    {
        Serial.begin(baudrate);
        return true;
    }

    int  available()
    {
        return Serial.available();
    }

    int  read(uint8_t* buffer, uint_t count)
    {
        return Serial.readBytes(buffer, count);
    }

    int  read()
    {
        return Serial.read();
    }

    int  write(const uint8_t* data, uint_t count)
    {
        return Serial.write(data, count);
    }

    int  write(uint8_t data)
    {
        return Serial.write(data);
    }

    int  write(const char* str)
    {
        return Serial.write(str);
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
    void delay(int ms)
    {
        ::delay(ms);
    }

    void printf(const char* fmt, ...)
    {
    }

    uint32_t millis()
    {
        return ::millis();
    }
};

template <int resetPin>
struct Avr
{
    void    begin()
    {
        pinMode(resetPin, OUTPUT);

        SPI.begin();
        SPI.setFrequency(300000);
        SPI.setHwCs(false);
    }

    bool    setDevice(const uint8_t* params)
    {
        if (params[0] != 0x14)  //ATTINY85
        {
            return false;
        }

        if (params[12] != 0)    // page size high
        {
            return false;
        }

        if (params[13] != 0x40) // page size low
        {
            return false;
        }

        return true;
    }

    bool    setDeviceExt(const uint8_t* params)
    {
        return true;
    }

    bool    enter()
    {
        digitalWrite(resetPin, LOW);
        SPI.transfer((uint8_t)0x00);

        for (int i = 0; i < 10; i ++)
        {
            digitalWrite(resetPin, HIGH);
            delayMicroseconds(50);
            digitalWrite(resetPin, LOW);
            delay(30);

            SPI.transfer(0xAC);
            SPI.transfer(0x53);
            uint8_t r = SPI.transfer(0x00);
            SPI.transfer(0x00);

            if (r == 0x53)
            {
                return true;
            }
        }

        return false;
    }

    void    leave()
    {
        digitalWrite(resetPin, HIGH);
    }

    uint8_t xfer(uint8_t b1, uint16_t b2b3 = 0, uint8_t b4 = 0)
    {
        return xfer(b1, b2b3 >> 8, b2b3 & 0xFF, b4);
    }

    uint8_t xfer(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
    {
        SPI.transfer(b1);
        SPI.transfer(b2);
        SPI.transfer(b3);
        return SPI.transfer(b4);
    }
};

#endif

