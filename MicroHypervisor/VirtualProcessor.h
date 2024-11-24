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
    HRESULT ConfigureVM(const VMConfig& config);

    /**
     * @brief Gets the Virtual machine configuration
     *
     */
    VMConfig GetVMConfig() const;

    /**
     * @brief Starts the Virtual Processor
     *
     */
    void Run();

    bool Continue();
    
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
    UINT64 GetSpecificRegister(WHV_REGISTER_NAME regName);

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

    /**
     * @brief Get the CPU Usage
     *
     */
    UINT64 GetCPUUsage();

    /**
     * @brief Get the Active Thread Count
     *
     */
    UINT GetActiveThreadCount();

    /**
	 * @brief Setup of the Kernel Memory
	 *
	 */
    bool SetupKernelMemory();

    /**
     * @brief Setup of the User Memory
     *
     */
    bool MapUserSpace();

private:
    UINT index_;
    WHV_PARTITION_HANDLE partitionHandle_;
    std::vector<WHV_REGISTER_VALUE> registers_;
    std::vector<WHV_REGISTER_VALUE> savedRegisters_;
    bool isRunning_;
    VMConfig vmConfig_;
    Logger logger_;

    struct Kernel 
    {
        uint64_t pml4[512];
        uint64_t pdpt[512];
    };

    const uint64_t kernel_start = 0x4000;
    const uint64_t user_start = 0x100000000;
    const uint64_t PTE_P = 1ULL << 0;
    const uint64_t PTE_RW = 1ULL << 1;
    const uint64_t PTE_US = 1ULL << 2;
    const uint64_t PTE_PS = 1ULL << 7;
    const uint8_t user_code[2][3] = { {0x0f, 0x01, 0xd9}, {0x0f, 0x01, 0xc1} };
    int vendor;
};

#endif // VIRTUALPROCESSOR_H