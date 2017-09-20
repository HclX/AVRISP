#ifndef __AVRISP_H__
#define __AVRISP_H__

#include "command.h"

struct NullPort
{
    int     available() { return 0; }
    int     read() { return -1; }
    int     read(uint8_t* buf, int size) { return -1; }
    int     write(uint8_t data) { return -1; }
    int     write(const uint8_t* data, int size) { return -1; }
    int     write(const char* str) { return -1; }
    void    discard() {};
};

struct NullAvr
{
    void    begin() {}
    bool    enter() { return false; }
    void    leave() {}
    uint8_t xfer(uint8_t b1, uint16_t b2b3 = 0, uint8_t b4 = 0) { return 0; }
};

struct NullSys
{
    void        printf(const char* fmt, ...) {}
    void        delay(int ms) {};
    uint32_t    millis() { return 0; }
};

template <typename TPort = NullPort, typename TAvr = NullAvr, typename TSys = NullSys>
class AVRISP
{
    TPort   _port;
    TAvr    _avr;
    TSys    _sys;

private:
    uint16_t    _cmd;
    uint_t      _cmdSize;
    uint8_t     _buf[256];
    uint_t      _bufIdx;
    uint32_t    _deadline;

private:
    uint16_t    _addr;

    enum
    {
        Meta_Cmnd_Min           = 0x1000,
        Meta_Cmnd_Idle          = 0x1000,
        Meta_Cmnd_Start         = 0x1100,
        Meta_Cmnd_Device_Ext    = 0x1200,
        Meta_Cmnd_Prog_Page     = 0x1300,
    };

private:
    void reset()
    {
        _cmd = Meta_Cmnd_Start;
        _cmdSize = 1;
        _bufIdx = 0;

        // next command should arrive in less than 5 seconds
        _deadline = _sys.millis() + 5000;
        _sys.printf("waiting for new packet\n");
    }

    void respCmd()
    {
        _sys.printf("Response:\n    ");
        for (auto i = 0; i < _cmdSize; i++)
        {
            _sys.printf("%02X ", _buf[i]);
            if ((i % 16) == 15)
            {
                _sys.printf("\n    ");
            }
        }
        _sys.printf("\n");

        if (_cmdSize != _port.write(_buf, _cmdSize))
        {
            _sys.printf("_port.write failed\n");
        }

        reset();
    }

    void syncError()
    {
        // when error happens, we discard all the pending commands, and return nosync error
        _port.discard();

        _buf[0] = Resp_STK_NOSYNC;
        _cmdSize = 1;

        respCmd();
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

    uint_t  receive()
    {
        uint_t n = _cmdSize - _bufIdx;
        int m = _port.available();

        if (m <= 0)
        {
            return 0;
        }

        if (m > n)
        {
            m = n;
        }

        m = _port.read(&_buf[_bufIdx], m);
        if (m <= 0)
        {
            _sys.printf("Unexpected, _port.read() returns <= 0 \n");
            return 0;
        }

        for (auto i = 0; i < m; i++)
        {
            _sys.printf("%02X ", _buf[_bufIdx + i]);
        }

        _bufIdx += m;
        return m;
    }

    void onMetaCmndStart()
    {
        switch (_buf[0])
        {
        case Cmnd_STK_GET_SIGN_ON:
        case Cmnd_STK_GET_SYNC:
        case Cmnd_STK_ENTER_PROGMODE:
        case Cmnd_STK_LEAVE_PROGMODE:
        case Cmnd_STK_CHIP_ERASE:
        case Cmnd_STK_CHECK_AUTOINC:
        case Cmnd_STK_PROG_LOCK:
        case Cmnd_STK_READ_FLASH:
        case Cmnd_STK_READ_DATA:
        case Cmnd_STK_READ_FUSE:
        case Cmnd_STK_READ_FUSE_EXT:
        case Cmnd_STK_READ_LOCK:
        case Cmnd_STK_READ_SIGN:
        case Cmnd_STK_READ_OSCCAL:
            _cmd = _buf[0];
            _bufIdx = 0;
            _cmdSize = 1;
            break;

        case Cmnd_STK_GET_PARAMETER:
        case Cmnd_STK_PROG_DATA:
        case Cmnd_STK_READ_OSCCAL_EXT:
            _cmd = _buf[0];
            _bufIdx = 0;
            _cmdSize = 2;
            break;

        case Cmnd_STK_SET_PARAMETER:
        case Cmnd_STK_LOAD_ADDRESS:
        case Cmnd_STK_PROG_FLASH:
        case Cmnd_STK_PROG_FUSE:
            _cmd = _buf[0];
            _bufIdx = 0;
            _cmdSize = 3;
            break;

        case Cmnd_STK_UNIVERSAL:
            _cmd = _buf[0];
            _bufIdx = 0;
            _cmdSize = 5;
            break;

        case Cmnd_STK_PROG_FUSE_EXT:
        case Cmnd_STK_READ_PAGE:
            _cmd = _buf[0];
            _bufIdx = 0;
            _cmdSize = 4;
            break;

        case Cmnd_STK_SET_DEVICE:
            _cmd = _buf[0];
            _bufIdx = 0;
            _cmdSize = 21;
            break;

        case Cmnd_STK_SET_DEVICE_EXT:
            _cmd = Meta_Cmnd_Device_Ext;
            _bufIdx = 0;
            _cmdSize = 1;
            break;

        case Cmnd_STK_PROG_PAGE:
            _cmd = Meta_Cmnd_Prog_Page;
            _bufIdx = 0;
            _cmdSize = 3;
            break;

        default:
            _sys.printf("Unknown command:%02X", _cmd);
            reset();
            break;
        }
    }

    void onSetParameter()
    {
        uint8_t id = _buf[0];
        uint8_t value = _buf[1];

        switch (id)
        {
        case Parm_STK_BUFSIZEL:
        case Parm_STK_BUFSIZEH:
        case Parm_STK_DEVICE:
        default:
            break;
        }

        syncOK();
    }

    void onGetParameter()
    {
        uint8_t value;
        switch (_buf[0])
        {
        case Parm_STK_HW_VER:
            value = '1';
            break;

        case Parm_STK_SW_MAJOR:
            value = '1';
            break;

        case Parm_STK_SW_MINOR:
            value = '1';
            break;

        case Parm_STK_BUFSIZEL:
        case Parm_STK_BUFSIZEH:
        case Parm_STK_DEVICE:
        default:
            value = 0;
            break;
        }

        syncOK(value);
    }

    void onSetDevice()
    {
        if (_avr.setDevice(_buf))
        {
            syncOK();
        }
        else
        {
            syncError();
        }
    }

    void onGetDevice()
    {
        syncOK();
    }

    void onSetDeviceExt()
    {
        syncOK();
    }

    void onProgPage()
    {
        auto size = (_buf[0] << 8) | _buf[1];
        _sys.printf("PROG PAGE: size=%08X, type=%c\n", size, _buf[2]);

        if (_buf[2] == 'F')
        {
            auto addr = _addr;
            auto data = &_buf[3];
            for (auto i = 0; i < size; i += 2)
            {
                _avr.xfer(0x40, addr, data[i]);
                _avr.xfer(0x48, addr, data[i + 1]);
                addr++;
            }

            _avr.xfer(0x4C, _addr);
            _sys.delay(5);

            syncOK();
            return;
        }

        if (_buf[2] == 'E')
        {
            auto addr = _addr;
            auto data = &_buf[3];
            for (auto i = 0; i < size; i++)
            {
                _avr.xfer(0xC1, addr, data[i]);
                addr++;
            }

            _avr.xfer(0xC2, _addr);
            _sys.delay(5);

            syncOK();
            return;
        }

        _sys.printf("Unknown flash type\n");
        syncError();
    }

    void onReadPage()
    {
        auto size = (_buf[0] << 8) | _buf[1];
        _sys.printf("READ PAGE: size=%08X, addr=%08X, type=%c\n",
            size, _addr, _buf[2]);

        if (size > sizeof(_buf) - 2)
        {
            syncError();
            return;
        }

        _buf[0] = Resp_STK_INSYNC;
        _buf[size + 1] = Resp_STK_OK;
        _cmdSize = size + 2;

        if (_buf[2] == 'F')
        {
            uint16_t addr = _addr;
            for (int i = 1; i < size + 1; i += 2)
            {
                _buf[i] = _avr.xfer(0x20, addr);
                _buf[i + 1] = _avr.xfer(0x28, addr);

                addr++;
            }

            respCmd();
            return;
        }

        if (_buf[2] == 'E')
        {
            uint16_t addr = _addr;
            for (int i = 1; i < size + 1; i++)
            {
                _buf[i] = _avr.xfer(0xA0, addr);
                addr++;
            }

            respCmd();
            return;
        }

        syncError();
    }

    void dumpCmd()
    {
        _sys.printf("\ncmd = %02X, size = %d\n", _cmd, _cmdSize, _cmdSize);
        if (_cmdSize > 0)
        {
            _sys.printf("data:");
            for (int i = 0; i < _cmdSize; i++)
            {
                _sys.printf("%02X, ", _buf[i]);
                if (i % 16 == 15)
                {
                    _sys.printf("\n     ");
                }
            }
            _sys.printf("\n");
        }
    }

    void updateInternal()
    {
        uint_t n = receive();

        if (n <= 0)
        {
            // nothing received, check timeout
            if (_cmd == Meta_Cmnd_Idle || _sys.millis() <= _deadline)
            {
                // Idle state has no timeout
                return;
            }

            if (_cmd == Meta_Cmnd_Start)
            {
                // Start state changes to Idle state
                _sys.printf("Timeout, entering idle state\n");
                _cmd = Meta_Cmnd_Idle;
            }
            else
            {
                // other state changes to start stae
                _sys.printf("Current packet timeout\n");
                reset();
            }

            return;
        }

        if (_bufIdx < _cmdSize)
        {
            // in the middle of packet, time out is set to 1 second
            _deadline = _sys.millis() + 1000;
            return;
        }

        if (_cmd < Meta_Cmnd_Min)
        {
            dumpCmd();
            if (_buf[_bufIdx - 1] != Sync_CRC_EOP)
            {
                // all real command packets should end with Sync_CRC_EOP
                _sys.printf("Incomplete command packet\n");

                syncError();
                return;
            }
        }

        switch (_cmd)
        {
        case Meta_Cmnd_Idle:
        case Meta_Cmnd_Start:
            onMetaCmndStart();
            break;

        case Meta_Cmnd_Device_Ext:
            _cmd = Cmnd_STK_SET_DEVICE_EXT;
            _cmdSize = _buf[0] + 1;
            _bufIdx = 1;
            if (_cmdSize > sizeof(_buf))
            {
                _sys.printf("Invalid data size for Cmnd_STK_SET_DEVICE_EXT: %d\n", _cmdSize);
                syncError();
            }
            break;

        case Meta_Cmnd_Prog_Page:
            _cmd = Cmnd_STK_PROG_PAGE;
            _cmdSize = ((_buf[0] << 8) | _buf[1]) + 4;
            _bufIdx = 3;

            if (_cmdSize > sizeof(_buf))
            {
                _sys.printf("Invalid data size for Cmnd_STK_PROG_PAGE: %d\n", _cmdSize);
                syncError();
            }
            break;

        case Cmnd_STK_GET_SYNC:           // 0x30  // ' '
        case Cmnd_STK_CHECK_AUTOINC:      // 0x53  // ' '
            syncOK();
            break;

        case Cmnd_STK_GET_SIGN_ON:        // 0x31  // ' '
            syncOK("STK");
            break;

        case Cmnd_STK_SET_PARAMETER:      // 0x40  // ' '
            onSetParameter();
            break;

        case Cmnd_STK_GET_PARAMETER:      // 0x41  // ' '
            onGetParameter();
            break;

        case Cmnd_STK_SET_DEVICE:         // 0x42  // ' '
            onSetDevice();
            break;

        case Cmnd_STK_GET_DEVICE:         // 0x43  // ' '
            onGetDevice();
            break;

        case Cmnd_STK_SET_DEVICE_EXT:     // 0x45  // ' '
            onSetDeviceExt();
            break;

        case Cmnd_STK_ENTER_PROGMODE:     // 0x50  // ' '
            if (_avr.enter())
            {
                syncOK();
            }
            else
            {
                syncError();
            }
            break;

        case Cmnd_STK_LEAVE_PROGMODE:     // 0x51  // ' '
            _avr.leave();
            syncOK();
            break;

        case Cmnd_STK_CHIP_ERASE:         // 0x52  // ' '
            _avr.xfer(0xAC, 0x8000);
            _sys.delay(10);

            syncOK();
            break;

        case Cmnd_STK_LOAD_ADDRESS:       // 0x55  // ' '
            _addr = (_buf[1] << 8) | _buf[0];
            _sys.printf("LOAD ADDRESS: 0x%04X\n", _addr);

            syncOK();
            break;

        case Cmnd_STK_UNIVERSAL:          // 0x56  // ' '
            _sys.printf("UNIVERSAL: %02X, %02X, %02X, %02X\n",
                _buf[0], _buf[1], _buf[2], _buf[3]);

            syncOK(_avr.xfer(_buf[0], _buf[1], _buf[2], _buf[3]));
            break;

        case Cmnd_STK_PROG_DATA:          // 0x61  // ' '
            _sys.printf("PROG DATA: %02X at %04X\n",
                _buf[0], _addr);

            _avr.xfer(0xC0, _addr, _buf[0]);
            _sys.delay(5);

            syncOK();
            break;

        case Cmnd_STK_PROG_FUSE:          // 0x62  // ' '
            _sys.printf("PROG FUSE: low=%02X, high=%02X",
                _buf[0], _buf[1]);

            _avr.xfer(0xAC, 0xA000, _buf[0]);
            _sys.delay(5);

            _avr.xfer(0xAC, 0xA800, _buf[1]);
            _sys.delay(5);

            syncOK();
            break;

        case Cmnd_STK_PROG_LOCK:          // 0x63  // ' '
            _sys.printf("PROG LOCK: %02X\n", _buf[0]);

            _avr.xfer(0xAC, 0xE000, _buf[0]);
            _sys.delay(5);

            syncOK();
            break;

        case Cmnd_STK_PROG_PAGE:          // 0x64  // ' '
            _sys.printf("PROG PAGE: size = %02X%02X, type = %c\n",
                _buf[0], _buf[1], _buf[2]);

            onProgPage();
            break;

        case Cmnd_STK_PROG_FUSE_EXT:      // 0x65  // ' '
            _sys.printf("PROG FUSE: low=%02X, high=%02X, ext=%02X",
                _buf[0], _buf[1], _buf[2]);

            _avr.xfer(0xAC, 0xA000, _buf[0]);
            _sys.delay(5);

            _avr.xfer(0xAC, 0xA800, _buf[1]);
            _sys.delay(5);

            _avr.xfer(0xAC, 0xA400, _buf[2]);
            _sys.delay(5);

            syncOK();
            break;

        case Cmnd_STK_READ_FLASH:         // 0x70  // ' '
            _sys.printf("READ FLASH @%02X\n", _addr);
            {
                auto r1 = _avr.xfer(0x28, _addr);
                auto r2 = _avr.xfer(0x20, _addr);

                syncOK((uint16_t)(r1 << 8 | r2));
            }
            break;

        case Cmnd_STK_READ_DATA:          // 0x71  // ' '
            _sys.printf("Read EEPROM @%02X\n", _addr);

            {
                auto r = _avr.xfer(0xA0, _addr);
                syncOK(r);
            }
            break;

        case Cmnd_STK_READ_FUSE:          // 0x72  // ' '
            _sys.printf("READ FUSE\n");

            {
                auto r1 = _avr.xfer(0x50);
                auto r2 = _avr.xfer(0x58);

                uint8_t fuses[] = { r1, r2 };
                syncOK(fuses, sizeof(fuses));
            }
            break;

        case Cmnd_STK_READ_LOCK:          // 0x73  // ' '
            _sys.printf("READ LOCK\n");
            {
                auto r = _avr.xfer(0x58);
                syncOK(r);
            }
            break;

        case Cmnd_STK_READ_PAGE:          // 0x74  // ' '
            _sys.printf("READ PAGE\n");
            onReadPage();
            break;

        case Cmnd_STK_READ_SIGN:          // 0x75  // ' '
            _sys.printf("READ SIGN\n");

            {
                auto r1 = _avr.xfer(0x30, 0x0000);
                auto r2 = _avr.xfer(0x30, 0x0001);
                auto r3 = _avr.xfer(0x30, 0x0002);

                uint8_t sig[] = { r1, r2, r3 };
                syncOK(sig, sizeof(sig));
            }
            break;

        case Cmnd_STK_READ_OSCCAL:        // 0x76  // ' '
            _sys.printf("READ OSCCAL\n");

            {
                auto r = _avr.xfer(0x38);
                syncOK(r);
            }
            break;

        case Cmnd_STK_READ_FUSE_EXT:      // 0x77  // ' '
            _sys.printf("READ FUSE EXT\n");

            {
                auto r1 = _avr.xfer(0x50, 0x0000);
                auto r2 = _avr.xfer(0x58, 0x0000);
                auto r3 = _avr.xfer(0x50, 0x0800);

                uint8_t fuses[] = { r1, r2, r3 };
                syncOK(fuses, sizeof(fuses));
            }
            break;

        case Cmnd_STK_READ_OSCCAL_EXT:    // 0x78  // ' '
        default:
            _sys.printf("Unknown command: %02X\n", _cmd);
            syncError();
            break;
        }
    }

    void beginInternal()
    {
        _port.begin(115200);
        _port.discard();

        _avr.begin();
        reset();
    }

public:
    AVRISP() {};
    ~AVRISP() {};

public:
    void begin() { beginInternal(); }
    void update() { updateInternal(); }
};

#endif

