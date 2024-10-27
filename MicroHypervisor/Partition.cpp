#include "Partition.h"
#include <iostream>

Partition::Partition() : handle_(nullptr)
{
    WHvCreatePartition(&handle_);
}

Partition::~Partition()
{
    WHvDeletePartition(handle_);
}

bool Partition::Setup() const
{
    WHV_PARTITION_PROPERTY property = {};
    property.ProcessorCount = 1;
    HRESULT result = WHvSetPartitionProperty(handle_, WHvPartitionPropertyCodeProcessorCount, &property, sizeof(property));
    return result == S_OK && WHvSetupPartition(handle_) == S_OK;
}

bool Partition::CreateVirtualProcessor(UINT index) const
{
    return WHvCreateVirtualProcessor(handle_, index, 0) == S_OK;
}

WHV_PARTITION_HANDLE Partition::GetHandle() const
{
    return handle_;
}
