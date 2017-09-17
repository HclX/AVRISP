// AVRISP_SIM.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "AVRISP.h"

int main()
{
    Serial.init("COM7");

    AVRISP avrisp;
    Serial.begin(115200);

    for (;;)
    {
        avrisp.update();
        Sleep(1);
    }

    return 0;
}

