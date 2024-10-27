#pragma once
#define _WIN32_WINNT 0x0A00
#include <Windows.h>
#include <map>
#include <Windows.h>
#include <WinHvEmulation.h>
#include <unordered_map>

/// @brief Memory Manager class for the Hypervisor \class MemoryManager
class MemoryManager
{
public:
    MemoryManager(WHV_PARTITION_HANDLE partitionHandle, size_t memorySize);
    ~MemoryManager();

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool Initialize();

    /**
     * @brief 
     * 
     * @param gva 
     * @return UINT64 
     */
    UINT64 TranslateGvaToGpa(UINT64 gva);

private:
    WHV_PARTITION_HANDLE partitionHandle_;
    size_t memorySize_;
    std::unordered_map<UINT64, UINT64> pageTable_;
};
