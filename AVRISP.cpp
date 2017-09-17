#include "stdafx.h"
#include "AVRISP.h"
#include "command.h"
#include "Platform.h"

const uint8_t default_params[] =
{
    '1',    //#define Parm_STK_HW_VER            0x80  // ' ' - R
    '1',    //#define Parm_STK_SW_MAJOR          0x81  // ' ' - R
    '1',    //#define Parm_STK_SW_MINOR          0x82  // ' ' - R
    '1',    //#define Parm_STK_LEDS              0x83  // ' ' - R/W
    '1',    //#define Parm_STK_VTARGET           0x84  // ' ' - R/W
    '1',    //#define Parm_STK_VADJUST           0x85  // ' ' - R/W
    '1',    //#define Parm_STK_OSC_PSCALE        0x86  // ' ' - R/W
    '1',    //#define Parm_STK_OSC_CMATCH        0x87  // ' ' - R/W
    '1',    //#define Parm_STK_RESET_DURATION    0x88  // ' ' - R/W
    '1',    //#define Parm_STK_SCK_DURATION      0x89  // ' ' - R/W

    0x00,   //#define Parm_STK_Unused_0x8A       0x8A  // '-' - N/A
    0x00,   //#define Parm_STK_Unused_0x8B       0x8B  // '-' - N/A
    0x00,   //#define Parm_STK_Unused_0x8C       0x8C  // '-' - N/A
    0x00,   //#define Parm_STK_Unused_0x8D       0x8D  // '-' - N/A
    0x00,   //#define Parm_STK_Unused_0x8E       0x8E  // '-' - N/A
    0x00,   //#define Parm_STK_Unused_0x8F       0x8F  // '-' - N/A

    '1',    //#define Parm_STK_BUFSIZEL          0x90  // ' ' - R/W, Range {0..255}
    '1',    //#define Parm_STK_BUFSIZEH          0x91  // ' ' - R/W, Range {0..255}
    '1',    //#define Parm_STK_DEVICE            0x92  // ' ' - R/W, Range {0..255}
    '1',    //#define Parm_STK_PROGMODE          0x93  // ' ' - 'P' or 'S'
   0x00,    //#define Parm_STK_PARAMODE          0x94  // ' ' - TRUE or FALSE
   0x00,    //#define Parm_STK_POLLING           0x95  // ' ' - TRUE or FALSE
   0x00,    //#define Parm_STK_SELFTIMED         0x96  // ' ' - TRUE or FALSE
};

class AvrIspImpl
{
    typedef void (AvrIspImpl::*handler_fn)();

    uint8_t     _buf[256];
    uint_t      _bufSize;

    uint_t      _cmdSize;
    uint_t      _cmdIdx;
    bool        _varSize;
    const char* _szCmd;
    handler_fn  _handler;

    uint8_t     _params[0x20];
    uint_t      _addr;

public:
    bool begin()
    {
        _bufSize = sizeof(_buf);
        _addr = 0;
        memset(_buf, 0, sizeof(_buf));
        memcpy(_params, default_params, sizeof(_params));

        endCmd();

        return true;
    }

    void beginCmd(uint8_t cmd, uint_t size, bool varSize, const char* szCmd, handler_fn handler)
    {
        _buf[0] = cmd;
        _cmdSize = size + 2;
        _cmdIdx = 1;
        _varSize = varSize;
        _szCmd = szCmd;
        _handler = handler;
    }

    void endCmd()
    {
        _cmdSize = 0;
        _cmdIdx = 0;
        _varSize = false;
        _szCmd = NULL;
        _handler = NULL;
    }

    void respCmd()
    {
        DbgPrintf("Response:\n    ");
        for (auto i = 0; i < _cmdSize; i++)
        {
            DbgPrintf("%02X ", _buf[i]);
            if ((i % 16) == 15)
            {
                DbgPrintf("\n    ");
            }
        }
        DbgPrintf("\n");

        auto res = Serial.write(_buf, _cmdSize);
        ASSERT(res == _cmdSize);

        endCmd();
    }

    void syncOK()
    {
        _buf[0] = Resp_STK_INSYNC;
        _buf[1] = Resp_STK_OK;
        _cmdSize = 2;

        respCmd();
    }

    void syncOK(const char* str)
    {
        int len = strlen(str);
        syncOK((const uint8_t*)str, len);
    }

    void syncOK(uint8_t data)
    {
        _buf[0] = Resp_STK_INSYNC;
        _buf[1] = data;
        _buf[2] = Resp_STK_OK;
        _cmdSize = 3;

        respCmd();
    }

    void syncOK(uint16_t data)
    {
        _buf[0] = Resp_STK_INSYNC;
        _buf[1] = data >> 8;
        _buf[2] = data & 0xFF;
        _buf[3] = Resp_STK_OK;
        _cmdSize = 4;

        respCmd();
    }

    void syncOK(const uint8_t* data, int size)
    {
        _buf[0] = Resp_STK_INSYNC;
        memcpy(&_buf[1], data, size);
        _buf[size + 1] = Resp_STK_OK;
        _cmdSize = size + 2;

        respCmd();
    }

    void syncRaw()
    {
        respCmd();
    }

    void syncError()
    {
        _buf[0] = Resp_STK_NOSYNC;
        _cmdSize = 1;

        respCmd();
    }

#define BEGIN_CMD(cmd) \
        switch (cmd) {\

#define DEFINE_CMD(id, size, vsize) \
        case id: \
            beginCmd(id, size, vsize, #id, &AvrIspImpl::onCmd_##id); \
        break;

#define DEFINE_CMD_NO_PAYLOAD(id) \
        case id: \
            beginCmd(id, 0, false, #id, &AvrIspImpl::onCmd_##id); \
        break;

#define DEFINE_CMD_FIXED_PAYLOAD(id, size) \
        case id: \
            beginCmd(id, size, false, #id, &AvrIspImpl::onCmd_##id); \
        break;

#define DEFINE_CMD_VAR_PAYLOAD(id, size) \
        case id: \
            beginCmd(id, size, true, #id, &AvrIspImpl::onCmd_##id); \
        break;

#define END_CMD() \
        default: \
            DbgPrintf("Unknown CMD:%02X\n", cmd); \
        break; \
        }

#define IMPL_CMD(cmd) \
        void onCmd_##cmd() 

    IMPL_CMD(Cmnd_STK_GET_SIGN_ON)
    {
        syncOK("AVR STK");
    }

    IMPL_CMD(Cmnd_STK_GET_SYNC)
    {
        syncOK();
    }

    IMPL_CMD(Cmnd_STK_ENTER_PROGMODE)
    {
        DbgPrintf("Entering PROG mode...\n");
        AVR.reset(true);
        syncOK();
    }

    IMPL_CMD(Cmnd_STK_LEAVE_PROGMODE)
    {
        DbgPrintf("Leaving PROG mode...\n");
        AVR.reset(false);
        syncOK();
    }

    IMPL_CMD(Cmnd_STK_CHIP_ERASE)
    {
        DbgPrintf("Chip erase...\n");

        SPI.xfer(0xAC800000);
        //TODO: wait for device

        syncOK();
    }

    IMPL_CMD(Cmnd_STK_CHECK_AUTOINC)
    {
        DbgPrintf("Check AutoInc\n");
        syncOK();
    }


    IMPL_CMD(Cmnd_STK_LOAD_ADDRESS)
    {
        _addr = (_buf[2] << 8) | _buf[1];

        DbgPrintf("LOAD ADDRESS: 0x%04X\n", _addr);
        syncOK();
    }

    IMPL_CMD(Cmnd_STK_PROG_FLASH)
    {
        uint16_t word = (_buf[2] << 8) | _buf[1];
        DbgPrintf("PROG FLASH: 0x%4X @ 0x%04X\n", word, _addr);

        SPI.xfer(0x4C000000 | (_addr << 8));
        SPI.xfer(word);

        //TODO: wait for device

        syncOK();
    }

    IMPL_CMD(Cmnd_STK_PROG_DATA)
    {
        DbgPrintf("PROG EEPROM: @%02X\n", _buf[1]);

        SPI.xfer(0xC0000000 | (_addr << 8) | _buf[1]);

        //TODO: wait for device

        syncOK();
    }

    IMPL_CMD(Cmnd_STK_PROG_FUSE)
    {
        DbgPrintf("PROG FUSE: low=%02X, high=%02X\n",
            _buf[1], _buf[2]);

        SPI.xfer(0xACA00000 | _buf[1]);
        SPI.xfer(0xACA80000 | _buf[2]);

        //TODO: wait for device

        syncOK();
    }

    IMPL_CMD(Cmnd_STK_PROG_FUSE_EXT)
    {
        DbgPrintf("PROG FUSE: low=%02X, high=%02X, ext=%02X\n",
            _buf[1], _buf[2], _buf[3]);

        SPI.xfer(0xACA00000 | _buf[1]);
        SPI.xfer(0xACA80000 | _buf[2]);
        SPI.xfer(0xACA40000 | _buf[3]);

        //TODO: wait for device

        syncOK();
    }

    IMPL_CMD(Cmnd_STK_PROG_LOCK)
    {
        DbgPrintf("PROG LOCK: %02X\n", _buf[1]);

        SPI.xfer(0xACE00000 | _buf[1]);

        syncOK();
    }

    IMPL_CMD(Cmnd_STK_PROG_PAGE)
    {
        auto size = (_buf[1] << 8) | _buf[2];
        DbgPrintf("PROG PAGE: size=%08X, type=%c\n", size, _buf[3]);

        if (_buf[3] == 'F')
        {
            auto addr = _addr;
            auto data = &_buf[4];
            for (auto i = 0; i < size; i += 2)
            {
                SPI.xfer(0x48000000 | (addr << 8) | data[i]);
                SPI.xfer(0x40000000 | (addr << 8) | data[i + 1]);
                addr ++;
            }

            SPI.xfer(0x4C000000 | (_addr << 8));
            syncOK();
        }
        else if (_buf[3] == 'E')
        {
            auto addr = _addr;
            auto data = &_buf[4];
            for (auto i = 0; i < size; i ++)
            {
                SPI.xfer(0xC1000000 | (addr << 8) | data[i]);
                addr++;
            }

            SPI.xfer(0xC2000000 | (_addr << 8));
            syncOK();
        }
        else
        {
            DbgPrintf("Unknown flash type\n");
            syncError();
        }
    }

    IMPL_CMD(Cmnd_STK_READ_FLASH)
    {
        DbgPrintf("Read flash @%02X\n", _addr);

        auto r1 = SPI.xfer(0x28000000 | (_addr << 8));
        auto r2 = SPI.xfer(0x20000000 | (_addr << 8));

        uint16_t data = ((r1 & 0xFF) << 8) | (r2 & 0xFF);

        syncOK(data);
    }

    IMPL_CMD(Cmnd_STK_READ_DATA)
    {
        DbgPrintf("Read EEPROM @%02X\n", _addr);

        auto r = SPI.xfer(0xA0000000 | (_addr << 8));

        syncOK((uint8_t) (r & 0xFF));
    }

    IMPL_CMD(Cmnd_STK_READ_FUSE)
    {
        DbgPrintf("Read fuse\n");

        auto r1 = SPI.xfer((uint32_t)0x50000000);
        auto r2 = SPI.xfer((uint32_t)0x58000000);

        uint8_t fuses[2] = { r1, r2};

        syncOK(fuses, sizeof(fuses));
    }

    IMPL_CMD(Cmnd_STK_READ_FUSE_EXT)
    {
        DbgPrintf("Read fuse\n");

        auto r1 = SPI.xfer((uint32_t)0x50000000);
        auto r2 = SPI.xfer((uint32_t)0x58000000);
        auto r3 = SPI.xfer((uint32_t)0x50080000);

        uint8_t fuses[3] = { r1, r2, r3 };
        syncOK(fuses, sizeof(fuses));
    }

    IMPL_CMD(Cmnd_STK_READ_LOCK)
    {
        DbgPrintf("Read lock\n");

        auto r = SPI.xfer((uint32_t)0x58000000);

        syncOK((uint8_t)r);
    }

    IMPL_CMD(Cmnd_STK_READ_SIGN)
    {
        DbgPrintf("Read signature\n");

        auto r1 = SPI.xfer((uint32_t)0x30000000);
        auto r2 = SPI.xfer((uint32_t)0x30000100);
        auto r3 = SPI.xfer((uint32_t)0x30000200);

        uint8_t sig[3] = { r1, r2, r3 };
        syncOK(sig, sizeof(sig));
    }

    IMPL_CMD(Cmnd_STK_READ_OSCCAL)
    {
        DbgPrintf("Read Oscillator Calibration byte\n");

        auto r = SPI.xfer((uint32_t)0x38000000);
        syncOK((uint8_t)r);
    }

    IMPL_CMD(Cmnd_STK_READ_OSCCAL_EXT)
    {
        DbgPrintf("Read Oscillator Calibration byte, not supported\n");
        syncError();
    }

    IMPL_CMD(Cmnd_STK_UNIVERSAL)
    {
        uint32_t cmd =
            (_buf[1] << 24) |
            (_buf[2] << 16) |
            (_buf[3] << 8) |
            _buf[4];

        DbgPrintf("UNIVERSAL BYTES: %08X\n", cmd);

        auto r = SPI.xfer(cmd);
        syncOK((uint8_t)r);
    }

    IMPL_CMD(Cmnd_STK_GET_PARAMETER)
    {
        auto id = _buf[1];
        DbgPrintf("Get Param, id = %02X\n", _buf[1]);
        
        if (id >= 0x80 && id <= 0x96)
        {
            syncOK(_params[id - 0x80]);
        }
        else
        {
            syncError();
        }
    }


    IMPL_CMD(Cmnd_STK_SET_PARAMETER)
    {
        auto id = _buf[1];
        auto value = _buf[2];

        DbgPrintf("Set Param, id = %02X, value = %02X\n",
            id, value);

        if (id >= 0x80 && id <= 0x96)
        {
            _params[id - 0x80] = value;
            syncOK();
        }
        else
        {
            syncError();
        }
    }

    IMPL_CMD(Cmnd_STK_READ_PAGE)
    {
        auto size = (_buf[1] << 8) | _buf[2];
        DbgPrintf("READ PAGE: size=%08X, addr=%08X, type=%c\n",
            size, _addr, _buf[3]);

        if (size > _bufSize - 2)
        {
            return syncError();
        }

        _buf[0] = Resp_STK_INSYNC;
        _buf[size + 1] = Resp_STK_OK;
        _cmdSize = size + 2;

        if (_buf[3] == 'F')
        {
            uint16_t addr = _addr;
            for (int i = 0; i < size; i +=2)
            {
                uint32_t data = 0x28000000 | (addr << 8);
                data = SPI.xfer(data);
                _buf[i + 1] = data & 0xFF;

                data = 0x20000000 | (addr << 8);
                data = SPI.xfer(data);
                _buf[i + 2] = data & 0xFF;

                addr++;
            }

            return syncRaw();
        }

        if (_buf[3] == 'E')
        {
            uint16_t addr = _addr;
            for (int i = 0; i < size; i++)
            {
                uint32_t data = 0xA0000000 | (addr << 8);
                data = SPI.xfer(data);
                _buf[i + 1] = data & 0xFF;

                addr++;
            }

            return syncRaw();
        }

        return syncError();
    }

    IMPL_CMD(Cmnd_STK_SET_DEVICE)
    {
        DbgPrintf("Set device\n");
        if (AVR.setDevice(&_buf[1]))
        {
            syncOK();
        }
        else
        {
            syncError();
        }
    }

    IMPL_CMD(Cmnd_STK_SET_DEVICE_EXT)
    {
        DbgPrintf("Set device\n");

        DbgPrintf("commandsize:    %02X\n", _buf[1]);
        DbgPrintf("eeprompagesize: %02X\n", _buf[2]);
        DbgPrintf("signalpagel:    %02X\n", _buf[3]);
        DbgPrintf("signalbs2:      %02X\n", _buf[4]);

        syncOK();
    }

    void onCmd(uint8_t cmd)
    {
        BEGIN_CMD(cmd)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_GET_SIGN_ON)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_GET_SYNC)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_ENTER_PROGMODE)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_LEAVE_PROGMODE)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_CHIP_ERASE)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_CHECK_AUTOINC)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_PROG_LOCK)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_READ_FLASH)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_READ_DATA)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_READ_FUSE)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_READ_FUSE_EXT)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_READ_LOCK)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_READ_SIGN)
            DEFINE_CMD_NO_PAYLOAD(Cmnd_STK_READ_OSCCAL)

            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_GET_PARAMETER, 0x01)
            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_PROG_DATA, 0x01)
            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_READ_OSCCAL_EXT, 0x01)
            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_SET_PARAMETER, 0x02)
            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_LOAD_ADDRESS, 0x02)
            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_PROG_FLASH, 0x02)
            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_PROG_FUSE, 0x02)
            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_UNIVERSAL, 0x04)
            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_PROG_FUSE_EXT, 0x03)
            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_READ_PAGE, 0x03)
            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_SET_DEVICE, 0x14)
            DEFINE_CMD_FIXED_PAYLOAD(Cmnd_STK_SET_DEVICE_EXT, 0x04)
            DEFINE_CMD_VAR_PAYLOAD(Cmnd_STK_PROG_PAGE, 0x03)
            END_CMD();
    }

    void dumpCmd()
    {
        DbgPrintf("\ncmd = %s(%02X), size = %d\n", _szCmd, _buf[0], _cmdSize - 2);
        if (_cmdSize > 2)
        {
            DbgPrintf("data:\n    ");
            for (int i = 1; i < _cmdSize - 1; i++)
            {
                DbgPrintf("%02X, ", _buf[i]);
                if (i % 16 == 0)
                {
                    DbgPrintf("\n    ");
                }
            }
            DbgPrintf("\n");
        }
    }

    void onData(uint8_t data)
    {
        if (_cmdSize == 0)
        {
            onCmd(data);
            return;
        }

        _buf[_cmdIdx ++] = data;
        if (_cmdIdx < _cmdSize)
        {
            return;
        }

        if (_varSize)
        {
            _cmdSize += ((_buf[1] << 8) | _buf[2]);
            _varSize = false;
        }

        if (_cmdIdx == _cmdSize)
        {
            dumpCmd();

            if (data != Sync_CRC_EOP)
            {
                syncError();
            }
            else
            {
                auto fn = _handler;
                (this->*fn)();
            }
        }
    }
};

AVRISP::AVRISP()
{
    _impl = new AvrIspImpl();
    ASSERT(_impl == NULL);
}

AVRISP::~AVRISP()
{
    if (_impl != NULL)
    {
        delete _impl;
        _impl = NULL;
    }
}

void AVRISP::update()
{
    while (Serial.available() > 0)
    {
        auto b = Serial.read();
        if (b < 0)
        {
            break;
        }

        DbgPrintf("%02X ", b);
        _impl->onData(b);
    }
}
