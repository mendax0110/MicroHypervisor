#pragma once
#define _WIN32_WINNT 0x0A00
#include <vector>
#include <Windows.h>
#include <WinHvPlatform.h>
#include <WinHvEmulation.h>

/// @brief Snapshot Manager class for the Hypervisor \class SnapshotManager
class SnapshotManager
{
public:
    SnapshotManager(WHV_PARTITION_HANDLE partitionHandle);
    ~SnapshotManager();

    /**
     * @brief 
     * 
     */
    void SaveSnapshot();

    /**
     * @brief 
     * 
     */
    void RestoreSnapshot();

    /**
     * @brief
     *
     */
    bool Initialize();

private:
    std::vector<WHV_REGISTER_VALUE> savedRegisters_;
    WHV_PARTITION_HANDLE partitionHandle_;
};
#pragma once
