#ifndef __SIM_AVR_H__
#define __SIM_AVR_H__

#include "Platform.h"

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

    bool setDevice(uint8_t* params);

    void reset(bool state);
    uint32_t xfer(uint32_t data);
};

#endif

