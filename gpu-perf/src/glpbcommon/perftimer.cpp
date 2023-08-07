#include <time.h>
#include <sys/time.h>

#include <perftimer.h>

PerfTimerPOSIX PerfTimer::timerPosix;
uint64_t PerfTimer::offset = 0;

void PerfTimer::initTimer()
{

#if defined(CLOCK_MONOTONIC)
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        timerPosix.monotonic = true;
        timerPosix.frequency = 1000000000;
    }
    else
#endif
    {
        timerPosix.monotonic = false;
        timerPosix.frequency = 1000000;
    }
    offset = _glfwPlatformGetTimerValue();
}

uint64_t PerfTimer::_glfwPlatformGetTimerValue(void)
{
#if defined(CLOCK_MONOTONIC)
    if (timerPosix.monotonic)
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t) ts.tv_sec * (uint64_t) 1000000000 + (uint64_t) ts.tv_nsec;
    }
    else
#endif
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (uint64_t) tv.tv_sec * (uint64_t) 1000000 + (uint64_t) tv.tv_usec;
    }
}

uint64_t PerfTimer::_glfwPlatformGetTimerFrequency(void)
{
    return timerPosix.frequency;
}

double PerfTimer::perfGetTime(void)
{
    uint64_t timerValue = _glfwPlatformGetTimerValue();
    uint64_t timerFrequency = _glfwPlatformGetTimerFrequency();
    double perfTime = static_cast<double>((timerValue - offset)) / timerFrequency;
    return perfTime;
}
