#pragma once
#include <stdint.h>

struct PerfTimerPOSIX
{
    bool monotonic;
    uint64_t frequency;
    PerfTimerPOSIX(){
        monotonic = false;
        frequency = 0;
    }
};

class PerfTimer
{
public:
    static void initTimer();
    static uint64_t _glfwPlatformGetTimerValue(void);
    static uint64_t _glfwPlatformGetTimerFrequency(void);
    static double perfGetTime(void);

public:
    static PerfTimerPOSIX timerPosix;
    static uint64_t offset;
};
