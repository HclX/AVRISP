#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#define __WIN32__

#if defined __ARDUINO__
#include <Arduino.h>
#endif // __ARDUINO

#if defined __WIN32__
#include <assert.h>
#include <Windows.h>
#include <vector>

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned int    uint_t;

#define DbgPrintf       printf

#define ASSERT(x)   assert(x)

#include "Win32Serial.h"
#include "SimAVR.h"

class Win32Serial;
class SimAVR;

extern Win32Serial Serial;
extern SimAVR      AVR;

#define SPI         AVR

#endif // __WIN32__
#endif // __PLATFORM_H__
