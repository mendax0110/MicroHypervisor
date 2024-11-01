#include "SnapshotManager.h"
#include <iostream>

SnapshotManager::SnapshotManager(WHV_PARTITION_HANDLE partitionHandle) : partitionHandle_(partitionHandle), logger_("SnapshotManager.log")
{

}

SnapshotManager::~SnapshotManager() {}

void SnapshotManager::SaveSnapshot()
{
    const int registerCount = 16;
    WHV_REGISTER_VALUE registerValues[registerCount];

    savedRegisters_.assign(registerValues, registerValues + registerCount);
    logger_.Log(Logger::LogLevel::Info, "Snapshot saved with " + std::to_string(registerCount) + " registers.");
}

void SnapshotManager::RestoreSnapshot()
{
    if (!savedRegisters_.empty())
    {
        auto result = WHvSetVirtualProcessorRegisters(
			partitionHandle_, 0, nullptr, 0, savedRegisters_.data()
		);
        logger_.Log(Logger::LogLevel::Info, "Restoring snapshot." 
            + std::to_string(result) + ", partitionHandle = "
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)) + ", registerCount = "
            + std::to_string(savedRegisters_.size()));
    }
    else
    {
        logger_.Log(Logger::LogLevel::Warning, "No snapshot available to restore.");
    }
}

bool SnapshotManager::Initialize()
{
    auto result = WHvGetVirtualProcessorRegisters(
		partitionHandle_, 0, nullptr, 0, nullptr
	);

    if (FAILED(result))
	{
        logger_.Log(Logger::LogLevel::Error, "Failed to initialize SnapshotManager: HRESULT "
            + std::to_string(result) + ", partitionHandle = "
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
        logger_.LogStackTrace();
		return false;
	}
    else
    {
        logger_.Log(Logger::LogLevel::Info, "SnapshotManager initialized successfully." 
			+ std::to_string(result) + ", partitionHandle = "
			+ std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
		return true;
    }
}
