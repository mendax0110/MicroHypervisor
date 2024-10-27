#include "MemoryManager.h"
#include <iostream>

MemoryManager::MemoryManager(WHV_PARTITION_HANDLE partitionHandle, size_t memorySize)
    : partitionHandle_(partitionHandle), memorySize_(memorySize), logger_("MemoryManager.log")
{

}

MemoryManager::~MemoryManager() {}

bool MemoryManager::Initialize()
{
    auto sysInfo = SYSTEM_INFO();
    GetSystemInfo(&sysInfo);
    auto pageSize = sysInfo.dwPageSize;
    if (pageSize == 0)
    {
        return false;
    }
    for (UINT64 i = 0; i < memorySize_; i += pageSize)
    {
        pageTable_[i] = i;
        std::cout << "Pagetable at: " << i << std::endl;
    }
    return true;
}

UINT64 MemoryManager::TranslateGvaToGpa(UINT64 gva)
{
    auto it = pageTable_.find(gva);
    if (it != pageTable_.end())
    {
        return it->second;
    }
    else
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to translate GVA: 0x%llx" + std::to_string(gva));
        return 0;
    }
}
