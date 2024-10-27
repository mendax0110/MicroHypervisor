#include "Timer.h"

Timer::Timer() : running_(false) {}

Timer::~Timer()
{
    Stop();
}

void Timer::Start(UINT32 intervalMilliseconds, std::function<void()> callback)
{
    running_ = true;
    timerThread_ = std::thread([this, intervalMilliseconds, callback]()
    {
        while (running_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMilliseconds));
            if (running_)
            {
                callback();
            }
        }
    });
}

void Timer::Stop()
{
    running_ = false;
    if (timerThread_.joinable())
    {
        timerThread_.join();
    }
}
