#include "WinImpl.h"

#define ATTINY85_SIGNATURE       "\x1E\x93\x0B"
#define ATMEGA328_SIGNATURE      "\x1E\x95\x0F"

bool SimAVR::setDevice(const uint8_t* params)
{
    printf("Set device\n");

    printf("devicecode:     %02X\n", params[0]);
    printf("revision:       %02X\n", params[1]);
    printf("progtype:       %02X\n", params[2]);
    printf("parmode:        %02X\n", params[3]);
    printf("polling:        %02X\n", params[4]);
    printf("selftimed:      %02X\n", params[5]);
    printf("lockbytes:      %02X\n", params[6]);
    printf("fusebytes:      %02X\n", params[7]);
    printf("flashpollval1:  %02X\n", params[8]);
    printf("flashpollval2:  %02X\n", params[9]);
    printf("eeprompollval1: %02X\n", params[10]);
    printf("eeprompollval2: %02X\n", params[11]);
    printf("pagesizehigh:   %02X\n", params[12]);
    printf("pagesizelow:    %02X\n", params[13]);
    printf("eepromsizehigh: %02X\n", params[14]);
    printf("eepromsizelow:  %02X\n", params[15]);
    printf("flashsize4:     %02X\n", params[16]);
    printf("flashsize3:     %02X\n", params[17]);
    printf("flashsize2:     %02X\n", params[18]);
    printf("flashsize1:     %02X\n", params[19]);

    auto pageSize = (params[12] << 8) | params[13];
    auto eepromSize = params[14] << 8 | params[15];
    auto flashSize = params[16] << 24 | params[17] << 16 | params[18] << 8 | params[19];
    auto deviceCode = params[0];

    switch (deviceCode)
    {
    case 0x14:  //ATTINY85
        _sig = ATTINY85_SIGNATURE;
        break;

    case 0x86:
        _sig = ATMEGA328_SIGNATURE;
        break;

    default:
        return false;
        break;
    }

    _flash.resize(flashSize / sizeof(uint16_t));
    _eeprom.resize(eepromSize);
    _page.resize(pageSize);

    memcpy(_params.data(), params, _params.size());

    return true;
}

bool SimAVR::setDeviceExt(const uint8_t* params)
{
    return true;
}

bool SimAVR::start()
{
    _reset = true;
    return true;
}

void SimAVR::finish()
{
    _reset = false;
}

uint8_t SimAVR::xfer(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    if (!_reset)
    {
        0xFF;
    }

    uint32_t data = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
    if (data == 0xAC530000)
    {
        _reset = true;
    }
    else if (data == 0xAC800000)
    {
        // chip erase
        memset(_flash.data(), 0xFF, _flash.size() * sizeof(uint16_t));
    }
    else if ((data & 0xFFFFFF00) == 0xF0000000)
    {
        // poll ready/busy
        data &= 0xFFFFFF00;
    }
    else if ((data & 0xFFFF00FF) == 0x4D000000)
    {
        // load extended address byte
        printf("Load Extended Address byte not supported\n");
        data = 0xFFFFFFFF;
    }
    else if ((data & 0xFF000000) == 0x48000000)
    {
        // load memory page, high byte
        uint16_t addr = (data >> 8) % _page.size();
        uint8_t hi = data;

        _page[addr] &= 0x00FF;
        _page[addr] |= (hi << 8);
    }
    else if ((data & 0xFF000000) == 0x40000000)
    {
        // load memory page, low byte
        uint16_t addr = (data >> 8) % _page.size();
        uint8_t lo = data;

        _page[addr] &= 0xFF00;
        _page[addr] |= (lo);
    }
    else if ((data & 0xFFFFFC00) == 0xC1000000)
    {
        // load eeprom memory page
        printf("Load eeprom memory page not supported\n");
        data = 0xFFFFFFFF;
    }
    else if ((data & 0xFF000000) == 0x28000000)
    {
        // read memory page, high byte
        uint16_t addr = data >> 8;
        uint8_t hi = _flash[addr] >> 8;

        data &= 0xFFFFFF00;
        data |= hi;
    }
    else if ((data & 0xFF000000) == 0x20000000)
    {
        // read memory page, lo byte
        uint16_t addr = data >> 8;
        uint8_t lo = _flash[addr] & 0xFF;

        data &= 0xFFFFFF00;
        data |= lo;
    }
    else if ((data & 0xFFFF0000) == 0xA0000000)
    {
        // read eeprom
        uint16_t addr = (data >> 8) & 0xFF;
        uint8_t b = _eeprom[addr];

        data &= 0xFFFFFF00;
        data |= b;
    }
    else if ((data & 0xFFFFFF00) == 0x58000000)
    {
        // read lock bits
        data &= 0xFFFFFF00;
        data |= _lock;
    }
    else if ((data & 0xFFFFFC00) == 0x30000000)
    {
        // read signature bits
        auto index = (data >> 8) & 0x03;

        data &= 0xFFFFFF00;
        data |= _sig[index];
    }
    else if ((data & 0xFFFFFF00) == 0x50000000)
    {
        // read fuse bits
        data &= 0xFFFFFF00;
        data |= _fuses[0];
    }
    else if ((data & 0xFFFFFF00) == 0x58080000)
    {
        // read fuse high bits
        data &= 0xFFFFFF00;
        data |= _fuses[1];
    }
    else if ((data & 0xFFFFFF00) == 0x50080000)
    {
        // read fuse ext bits
        data &= 0xFFFFFF00;
        data |= _fuses[2];
    }
    else if ((data & 0xFFFFFF00) == 0x38000000)
    {
        // read calibration byte
        data &= 0xFFFFFF00;
        data |= _cal;
    }
    else if ((data & 0xFF0000FF) == 0x4C000000)
    {
        // write progrm page
        uint16_t addr = ((data >> 8) / _page.size()) * _page.size();
        printf("Programming @%08X\n", addr);
        memcpy(_flash.data() + addr, _page.data(), _page.size() * sizeof(uint16_t));
    }
    else if ((data & 0xFFFF0000) == 0xC0000000)
    {
        // write eeprom
        uint16_t addr = data >> 8;
        uint8_t b = data & 0xFF;
        _eeprom[addr] = b;
    }
    else if ((data & 0xFFFFFF00) == 0xACE00000)
    {
        // write lock bits
        _lock = data & 0xFF;
    }
    else if ((data & 0xFFFFFF00) == 0xACA00000)
    {
        // write fuse bits
        _fuses[0] = data & 0xFF;
    }
    else if ((data & 0xFFFFFF00) == 0xACA80000)
    {
        // write fuse high bits
        _fuses[1] = data & 0xFF;
    }
    else if ((data & 0xFFFFFF00) == 0xACA40000)
    {
        // write fuse ext bits
        _fuses[2] = data & 0xFF;
    }
    else
    {
        printf("Unsupported SPI instruction: %08X\n", data);
        data = 0xFFFFFFFF;
    }

    return (uint8_t)data;
}

uint8_t SimAVR::xfer(uint8_t b1, uint16_t b2b3, uint8_t b4)
{
    return xfer(b1, (uint8_t)(b2b3 >> 8), (uint8_t)(b2b3 & 0xFF), b4);
}