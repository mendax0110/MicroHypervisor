#ifndef EMULATOR_H
#define EMULATOR_H

#include <Windows.h>
#include <WinHvEmulation.h>
#include "Logger.h"

/// @brief Emulator class for the Hypervisor \class Emulator
class Emulator
{
public:
    Emulator();
    ~Emulator();

    /**
     * @brief  
     * 
     * @return true 
     * @return false 
     */
    bool Initialize();

private:
    WHV_EMULATOR_HANDLE handle_;
    WHV_EMULATOR_CALLBACKS callbacks_;
    Logger logger_;
};


#endif // EMULATOR_H