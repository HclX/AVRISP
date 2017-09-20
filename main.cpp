// AVRISP_SIM.cpp : Defines the entry point for the console application.
//

#include "WinImpl.h"
#include "AVRISP.h"

int main()
{
    AVRISP<Win32Serial<7>, SimAVR, Sys> avrisp;

    avrisp.begin();

    for (;;)
    {
        avrisp.update();
        Sleep(1);
    }

    return 0;
}

