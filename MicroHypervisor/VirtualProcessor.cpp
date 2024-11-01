#include "VirtualProcessor.h"
#include <iostream>
#include <iomanip>
#include "InterruptController.h"

VirtualProcessor::VirtualProcessor(WHV_PARTITION_HANDLE partitionHandle, UINT index)
    : partitionHandle_(partitionHandle), index_(index), registers_(std::size(regNames)), 
    isRunning_(false), savedRegisters_(), vmConfig_({1, 4194304, "none"}), logger_("VirtualProcessor.log")
{}

VirtualProcessor::~VirtualProcessor()
{
    WHvDeleteVirtualProcessor(partitionHandle_, index_);
}

HRESULT VirtualProcessor::GetRegisters()
{
    auto result = WHvGetVirtualProcessorRegisters(partitionHandle_, index_, regNames, std::size(regNames), registers_.data());
    if (FAILED(result))
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to get registers: HRESULT " 
            + std::to_string(result) + ", index = " 
            + std::to_string(index_) + ", partitionHandle = " 
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
        return result;
    }
    else
    {
        logger_.Log(Logger::LogLevel::Info, "Registers retrieved successfully." 
            + std::to_string(result) + ", index = " 
            + std::to_string(index_) + ", partitionHandle = "
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
		return result;
    }
}

HRESULT VirtualProcessor::SetRegisters()
{
    auto result = WHvSetVirtualProcessorRegisters(partitionHandle_, index_, regNames, std::size(regNames), registers_.data());
    if (FAILED(result))
	{
		logger_.Log(Logger::LogLevel::Error, "Failed to set registers: HRESULT " 
            + std::to_string(result) + ", index = " 
            + std::to_string(index_) + ", partitionHandle = "
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
        return result;
	}
    else
    {
        logger_.Log(Logger::LogLevel::Info, "Registers set successfully." 
            + std::to_string(result) + ", index = " 
            + std::to_string(index_) + ", partitionHandle = "
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
        return result;
    }
}

HRESULT VirtualProcessor::SetSpecificRegister(WHV_REGISTER_NAME regName, UINT64 value)
{
    for (size_t i = 0; i < std::size(regNames); ++i)
    {
        if (regNames[i] == regName)
        {
            registers_[i].Reg64 = value;
            auto result = WHvSetVirtualProcessorRegisters(partitionHandle_, index_, &regName, 1, &registers_[i]);
            if (FAILED(result))
            {
                logger_.Log(Logger::LogLevel::Error, "Failed to set specific register: HRESULT " 
                    + std::to_string(result) + ", regName = " 
                    + std::to_string(regName) + ", value = "
                    + std::to_string(value) + ", index = "
                    + std::to_string(index_) + ", partitionHandle = "
                    + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
				return result;
            }
            else
            {
                logger_.Log(Logger::LogLevel::Info, "Specific register set successfully." 
                    + std::to_string(result) + ", regName = " 
                    + std::to_string(regName) + ", value = "
                    + std::to_string(value) + ", index = "
                    + std::to_string(index_) + ", partitionHandle = "
                    + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
                return result;
            }
        }
    }
    return E_INVALIDARG;
}

UINT64 VirtualProcessor::GetSpecificRegister(WHV_REGISTER_NAME regName)
{
    for (size_t i = 0; i < std::size(regNames); ++i)
    {
        if (regNames[i] == regName)
        {
            return registers_[i].Reg64;
            auto result = WHvGetVirtualProcessorRegisters(partitionHandle_, index_, &regName, 1, &registers_[i]);
            if (FAILED(result))
            {
                logger_.Log(Logger::LogLevel::Error, "Failed to get specific register: HRESULT " 
                    + std::to_string(result) + ", regName = " 
                    + std::to_string(regName) + ", index = "
                    + std::to_string(index_) + ", partitionHandle = "
                    + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
                return registers_[i].Reg64;
            }
            else
            {
                logger_.Log(Logger::LogLevel::Info, "Specific register retrieved successfully." 
                    + std::to_string(result) + ", regName = " 
                    + std::to_string(regName) + ", index = "
                    + std::to_string(index_) + ", partitionHandle = "
                    + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
                return registers_[i].Reg64;
            }
        }
    }
    return 0;
}

void VirtualProcessor::DumpRegisters()
{
    GetRegisters();
    std::cout << "Register Dump: \n";
    for (const auto& reg : registers_)
    {
        std::cout << "Reg = " << std::hex << reg.Reg64 << std::dec << "\n";
    }
}

void VirtualProcessor::DetailedDumpRegisters()
{
    GetRegisters();
    std::cout << "----------------------------------------------------------------------------" << std::endl;
    std::cout << "Detailed Register Dump: \n";
    for (const auto& reg : registers_)
    {
        std::cout << "Reg = " << std::hex << reg.Reg64 << "\n";
    }
    std::cout << "----------------------------------------------------------------------------" << std::endl;

    std::cout << "Segment Registers: \n";
    std::cout << "----------------------------------------------------------------------------" << std::endl;
    std::cout << "ES = " << std::hex << GetSpecificRegister(WHvX64RegisterEs) << std::dec << "\n";
    std::cout << "CS = " << std::hex << GetSpecificRegister(WHvX64RegisterCs) << std::dec << "\n";
    std::cout << "SS = " << std::hex << GetSpecificRegister(WHvX64RegisterSs) << std::dec << "\n";
    std::cout << "DS = " << std::hex << GetSpecificRegister(WHvX64RegisterDs) << std::dec << "\n";
    std::cout << "FS = " << std::hex << GetSpecificRegister(WHvX64RegisterFs) << std::dec << "\n";
    std::cout << "GS = " << std::hex << GetSpecificRegister(WHvX64RegisterGs) << std::dec << "\n";
    std::cout << "----------------------------------------------------------------------------" << std::endl;

    std::cout << "Control Registers: \n";
    std::cout << "----------------------------------------------------------------------------" << std::endl;
    std::cout << "GDTR = " << std::hex << GetSpecificRegister(WHvX64RegisterGdtr) << std::dec << "\n";
    std::cout << "CR0 = " << std::hex << GetSpecificRegister(WHvX64RegisterCr0) << std::dec << "\n";
    std::cout << "CR2 = " << std::hex << GetSpecificRegister(WHvX64RegisterCr2) << std::dec << "\n";
    std::cout << "CR3 = " << std::hex << GetSpecificRegister(WHvX64RegisterCr3) << std::dec << "\n";
    std::cout << "CR4 = " << std::hex << GetSpecificRegister(WHvX64RegisterCr4) << std::dec << "\n";
    std::cout << "CR8 = " << std::hex << GetSpecificRegister(WHvX64RegisterCr8) << std::dec << "\n";
    std::cout << "EFER = " << std::hex << GetSpecificRegister(WHvX64RegisterEfer) << std::dec << "\n";
    std::cout << "LSTAR = " << std::hex << GetSpecificRegister(WHvX64RegisterLstar) << std::dec << "\n";
    std::cout << "Pending Interruption = " << std::hex << GetSpecificRegister(WHvRegisterPendingInterruption) << std::dec << "\n";
    std::cout << "----------------------------------------------------------------------------" << std::endl;
    
    std::cout << "Other Registers: \n";
    std::cout << "----------------------------------------------------------------------------" << std::endl;
    std::cout << "RIP = " << std::hex << GetSpecificRegister(WHvX64RegisterRip) << std::dec << "\n";
    std::cout << "RFLAGS = " << std::hex << GetSpecificRegister(WHvX64RegisterRflags) << std::dec << "\n";
    std::cout << "----------------------------------------------------------------------------" << std::endl;
}

HRESULT VirtualProcessor::SaveState()
{
    HRESULT hr = GetRegisters();
    if (FAILED(hr)) return hr;

    savedRegisters_ = registers_;
    isRunning_ = true;

    auto result = WHvGetVirtualProcessorRegisters(partitionHandle_, index_, regNames, std::size(regNames), savedRegisters_.data());
    if (FAILED(result))
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to save state: HRESULT " 
            + std::to_string(result) + ", index = "
            + std::to_string(index_) + ", partitionHandle = "
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
		return result;
    }
    else
    {
        logger_.Log(Logger::LogLevel::Info, "State saved successfully." 
            + std::to_string(result) + ", index = "
            + std::to_string(index_) + ", partitionHandle = "
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
    }

    return S_OK;
}

HRESULT VirtualProcessor::RestoreState()
{
    if (!isRunning_) return E_FAIL;

    registers_ = savedRegisters_;

    auto result = WHvSetVirtualProcessorRegisters(partitionHandle_, index_, regNames, std::size(regNames), registers_.data());
    if (FAILED(result))
	{
		logger_.Log(Logger::LogLevel::Error, "Failed to restore state: HRESULT " 
            + std::to_string(result) + ", index = "
            + std::to_string(index_) + ", partitionHandle = "
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
		return result;
	}
    else
    {
        logger_.Log(Logger::LogLevel::Info, "State restored successfully."
            + std::to_string(result) + ", index = "
            + std::to_string(index_) + ", partitionHandle = "
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
    }

    return SetRegisters();
}

void VirtualProcessor::ConfigureVM(const VMConfig& config)
{
    vmConfig_ = config;
    std::cout << "VM configured with "
        << vmConfig_.cpuCount << " CPU(s), "
        << vmConfig_.memorySize << " bytes of memory, "
        << "I/O devices: " << vmConfig_.ioDevices << std::endl;
}

VirtualProcessor::VMConfig VirtualProcessor::GetVMConfig() const
{
    return vmConfig_;
}

UINT64 VirtualProcessor::GetCPUUsage()
{
    UINT64 cpuUsage = 0;
    //HRESULT result = WHvGetVirtualProcessorCounters(partitionHandle_, index_, WHV_PROCESSOR_COUNTER_SET(0), 0, cpuUsage, nullptr);

    //if (FAILED(result))
    //{
    //    logger_.Log(Logger::LogLevel::Error, "Failed to get CPU usage: HRESULT "
    //        + std::to_string(result) + ", index = "
    //        + std::to_string(index_) + ", partitionHandle = "
    //        + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
    //    return 0;
    //}

    //logger_.Log(Logger::LogLevel::Info, "CPU usage retrieved successfully: "
    //    + std::to_string(cpuUsage) + ", index = "
    //    + std::to_string(index_) + ", partitionHandle = "
    //    + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));

    return cpuUsage;
}

UINT VirtualProcessor::GetActiveThreadCount()
{
    UINT32 activeThreadCount = 0;
    //HRESULT result = WHvGetVirtualProcessorCounters(partitionHandle_, index_, WHV_PROCESSOR_COUNTER_SET(1), 0, activeThreadCount, nullptr);

    //if (FAILED(result))
    //{
    //    logger_.Log(Logger::LogLevel::Error, "Failed to get active thread count: HRESULT "
    //        + std::to_string(result) + ", index = "
    //        + std::to_string(index_) + ", partitionHandle = "
    //        + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
    //    return 0;
    //}

    //logger_.Log(Logger::LogLevel::Info, "Active thread count retrieved successfully: "
    //    + std::to_string(activeThreadCount) + ", index = "
    //    + std::to_string(index_) + ", partitionHandle = "
    //    + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));

    return activeThreadCount;
}

void VirtualProcessor::Run()
{
    //TODO FIX THIS!!!!
    //SetRegisters();
    SetSpecificRegister(WHvX64RegisterRip, 0x1000);
    SetSpecificRegister(WHvX64RegisterRflags, 0x2);
    SetSpecificRegister(WHvX64RegisterCs, 0x8);
    
    if (partitionHandle_ == nullptr)
    {
        logger_.Log(Logger::LogLevel::Error, "Partition handle is invalid. Cannot run virtual processor.");
        return;
    }

    //TODO FIX THIS!!!!
    auto result = WHvRunVirtualProcessor(partitionHandle_, index_, 0, 0);

    if (result == S_OK)
	{
		logger_.Log(Logger::LogLevel::Info, "Virtual Processor is running.");

		// Inject an interrupt
		InterruptController interruptController(partitionHandle_);
		if (interruptController.Setup())
		{
			interruptController.InjectInterrupt(0x20);
		}
		else
		{
			logger_.Log(Logger::LogLevel::Error, "Failed to setup Interrupt Controller.");
			logger_.LogStackTrace();
        }
    }
    else
    {
        logger_.Log(Logger::LogLevel::Error, "Virtual Processor failed to run." + std::to_string(result));
        logger_.LogStackTrace();
    }
}