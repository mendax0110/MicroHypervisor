#pragma once
#define _WIN32_WINNT 0x0A00
#include <Windows.h>
#include <chrono>
#include <thread>
#include <functional>

/// @brief Timer class for the Hypervisor \class Timer
class Timer
{
public:
    Timer();
    ~Timer();

    /**
     * @brief 
     * 
     * @param intervalMilliseconds 
     * @param callback 
     */
    void Start(UINT32 intervalMilliseconds, std::function<void()> callback);
    
    /**
     * @brief 
     * 
     */
    void Stop();

private:
    std::thread timerThread_;
    bool running_;
};
