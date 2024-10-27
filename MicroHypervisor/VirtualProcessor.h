#ifndef VIRTUALPROCESSOR_H
#define VIRTUALPROCESSOR_H

#include <Windows.h>
#include <WinHvPlatform.h>
#include <WinHvEmulation.h>
#include "Registers.h"
#include <vector>
#include <array>
#include <string>
#include "Logger.h"

/// @brief  Virtual Processor class for the Hypervisor \class VirtualProcessor
class VirtualProcessor
{
public:
    VirtualProcessor(WHV_PARTITION_HANDLE partitionHandle, UINT index);
    ~VirtualProcessor();

    /**
     * @brief struct of the Virtual machine conf
     * 
     */
    struct VMConfig
    {
        size_t cpuCount;
        size_t memorySize;
        std::string ioDevices;
    };

    /**
     * @brief Configures the Virtual Machine
     *
     * @param config -> VMConfig, the configuration of the Virtual Machine
     */
    void ConfigureVM(const VMConfig& config);


    /**
     * @brief Gets the Virtual machine configuration
     *
     */
    VMConfig GetVMConfig() const;

    void Run();
    
    /**
     * @brief Dump the Registers
     * 
     */
    void DumpRegisters();

    /**
     * @brief Dump a detailed view of the Registers
     * 
     */
    void DetailedDumpRegisters();

    /**
     * @brief Set the Registers object
     * 
     * @return HRESULT -> S_OK if successful
     */
    HRESULT SetRegisters();

    /**
     * @brief Get the Registers object
     * 
     * @return HRESULT -> S_OK if successful
     */
    HRESULT GetRegisters();

    /**
     * @brief Set the Specific Register object
     * 
     * @param regName -> WHV_REGISTER_NAME
     * @param value -> UINT64
     * @return HRESULT -> S_OK if successful
     */
    HRESULT SetSpecificRegister(WHV_REGISTER_NAME regName, UINT64 value);

    /**
     * @brief Get the Specific Register object
     * 
     * @param regName -> WHV_REGISTER_NAME
     * @return UINT64 -> Register value
     */
    UINT64 GetSpecificRegister(WHV_REGISTER_NAME regName) const;

    /**
     * @brief Save the state of the Virtual Processor
     * 
     * @return HRESULT -> S_OK if successful
     */
    HRESULT SaveState();

    /**
     * @brief Restore the state of the Virtual Processor
     * 
     * @return HRESULT -> S_OK if successful
     */
    HRESULT RestoreState();

private:
    UINT index_;
    WHV_PARTITION_HANDLE partitionHandle_;
    std::vector<WHV_REGISTER_VALUE> registers_;
    std::vector<WHV_REGISTER_VALUE> savedRegisters_;
    bool isRunning_;
    VMConfig vmConfig_;
    Logger logger_;
};

#endif // VIRTUALPROCESSOR_H