#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <Windows.h>
#include <WinHvEmulation.h>
#include <unordered_map>
#include "Logger.h"

/// @brief Memory Manager class for the Hypervisor \class MemoryManager
class MemoryManager
{
public:
    MemoryManager(WHV_PARTITION_HANDLE partitionHandle, size_t memorySize);
    ~MemoryManager();

    /**
     * @brief Initializes the Memory Manager
     *
     * @return true -> if the Memory Manager is initialized successfully
     * @return false -> if the Memory Manager initialization fails
     */
    bool Initialize();

    /**
     * @brief Translates the Guest Virtual Address to Guest Physical Address
     *
     * @param gva -> UINT64, Guest Virtual Address for translation
     * @return UINT64 -> Guest Physical Address, if translation is successful
     */
    UINT64 TranslateGvaToGpa(UINT64 gva);

    /**
     * @brief Updates the Memory Size of the Partition
     *
     * @param newMemorySize -> size_t, New Memory Size
     */
    void UpdateMemorySize(size_t newMemorySize);

    // method GetCurrentUsage()
    UINT64 GetCurrentUsage();

private:
    WHV_PARTITION_HANDLE partitionHandle_;
    size_t memorySize_;
    std::unordered_map<UINT64, UINT64> pageTable_;
    Logger logger_;
};

#endif // MEMORY_MANAGER_H