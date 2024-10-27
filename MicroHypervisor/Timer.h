#ifndef TIMER_H
#define TIMER_H

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
     * @brief Start the Timer with a specified interval and callback
     * 
     * @param intervalMilliseconds -> The interval in milliseconds
     * @param callback ->  
     */ 
    void Start(UINT32 intervalMilliseconds, std::function<void()> callback);
    
    /**
     * @brief Stops the Timer
     * 
     */
    void Stop();

private:
    std::thread timerThread_;
    bool running_;
};

#endif // TIMER_H