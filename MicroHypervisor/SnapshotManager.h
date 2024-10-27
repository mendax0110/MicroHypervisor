#ifndef SNAPSHOTMANAGER_H
#define SNAPSHOTMANAGER_H

#include <vector>
#include <Windows.h>
#include <WinHvPlatform.h>
#include <WinHvEmulation.h>
#include "Logger.h"

/// @brief Snapshot Manager class for the Hypervisor \class SnapshotManager
class SnapshotManager
{
public:
    SnapshotManager(WHV_PARTITION_HANDLE partitionHandle);
    ~SnapshotManager();

    /**
     * @brief Saves a snapshot of the current state of the partition
     * 
     */
    void SaveSnapshot();

    /**
     * @brief Restores the snapshot of the partition
     * 
     */
    void RestoreSnapshot();

    /**
     * @brief Initializes the Snapshot Manager
     *
     */
    bool Initialize();

private:
    std::vector<WHV_REGISTER_VALUE> savedRegisters_;
    WHV_PARTITION_HANDLE partitionHandle_;
    Logger logger_;
};

#endif // SNAPSHOTMANAGER_H