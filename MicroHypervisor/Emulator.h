#pragma once
#define _WIN32_WINNT 0x0A00
#include <Windows.h>
#include <WinHvEmulation.h>

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
};
