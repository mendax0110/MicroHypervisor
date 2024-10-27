#pragma once
#define _WIN32_WINNT 0x0A00
#include <Windows.h>
#include <WinHvPlatform.h>
#include "VirtualProcessor.h"

/// @brief Partition class for the Hypervisor \class Partition
class Partition
{
public:
    Partition();
    ~Partition();

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool Setup() const;

    /**
     * @brief Create a Virtual Processor object
     * 
     * @param index 
     * @return true 
     * @return false 
     */
    bool CreateVirtualProcessor(UINT index) const;

    /**
     * @brief Get the Handle object
     * 
     * @return WHV_PARTITION_HANDLE 
     */
    WHV_PARTITION_HANDLE GetHandle() const;

private:
    WHV_PARTITION_HANDLE handle_;
};
