#pragma once
#define _WIN32_WINNT 0x0A00
#include <Windows.h>
#include <WinHvPlatform.h>
#include <WinHvPlatformDefs.h>
#include <vector>

/// @brief Interrupt Controller class for the Hypervisor \class InterruptController
class InterruptController
{
public:
    InterruptController(WHV_PARTITION_HANDLE partitionHandle);
    ~InterruptController();

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool Setup();

    /**
     * @brief 
     * 
     * @param interruptVector 
     */
    void InjectInterrupt(UINT32 interruptVector) const;

private:
    WHV_PARTITION_HANDLE partitionHandle_;
    std::vector<WHV_REGISTER_VALUE> interruptRegisters_;
};
