#ifndef __AVRISP_H__
#define __AVRISP_H__

#include "Platform.h"

class AvrIspImpl;

class AVRISP
{
    AvrIspImpl* _impl;

public:
    AVRISP();
    ~AVRISP();

public:
    void begin();
    void update();
};

#endif

