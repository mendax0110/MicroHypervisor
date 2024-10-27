#ifndef PARTITION_H
#define PARTITION_H

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
     * @brief Function to setup the partition
     * 
     * @return true -> if the partition is setup successfully
     * @return false -> if the partition setup fails
     */
    bool Setup() const;

    /**
     * @brief Create a Virtual Processor object
     * 
     * @param index -> UINT, index of the Virtual Processor
     * @return true -> if the Virtual Processor is created successfully
     * @return false -> if the Virtual Processor creation fails
     */
    bool CreateVirtualProcessor(UINT index) const;

    /**
     * @brief Get the Handle object
     * 
     * @return WHV_PARTITION_HANDLE -> Handle to the Partition
     */
    WHV_PARTITION_HANDLE GetHandle() const;

private:
    WHV_PARTITION_HANDLE handle_;
};

#endif // PARTITION_H