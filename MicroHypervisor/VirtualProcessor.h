#pragma once
#define _WIN32_WINNT 0x0A00
#include <Windows.h>
#include <WinHvPlatform.h>
#include <WinHvEmulation.h>
#include "Registers.h"
#include <vector>
#include <array>

/// @brief  Virtual Processor class for the Hypervisor \class VirtualProcessor
class VirtualProcessor
{
public:
    VirtualProcessor(WHV_PARTITION_HANDLE partitionHandle, UINT index);
    ~VirtualProcessor();

    /**
     * @brief 
     * 
     */
    void DumpRegisters();

    /**
     * @brief 
     * 
     */
    void DetailedDumpRegisters();

    /**
     * @brief Set the Registers object
     * 
     * @return HRESULT 
     */
    HRESULT SetRegisters();

    /**
     * @brief Get the Registers object
     * 
     * @return HRESULT 
     */
    HRESULT GetRegisters();

    /**
     * @brief Set the Specific Register object
     * 
     * @param regName 
     * @param value 
     * @return HRESULT 
     */
    HRESULT SetSpecificRegister(WHV_REGISTER_NAME regName, UINT64 value);

    /**
     * @brief Get the Specific Register object
     * 
     * @param regName 
     * @return UINT64 
     */
    UINT64 GetSpecificRegister(WHV_REGISTER_NAME regName) const;

    /**
     * @brief 
     * 
     * @return HRESULT 
     */
    HRESULT SaveState();

    /**
     * @brief 
     * 
     * @return HRESULT 
     */
    HRESULT RestoreState();

private:
    UINT index_;
    WHV_PARTITION_HANDLE partitionHandle_;
    std::vector<WHV_REGISTER_VALUE> registers_;
    std::vector<WHV_REGISTER_VALUE> savedRegisters_;
    bool isRunning_;
};
