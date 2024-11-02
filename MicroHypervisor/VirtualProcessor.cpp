#include "VirtualProcessor.h"
#include <iostream>
#include <iomanip>
#include "InterruptController.h"
#include <cassert>
#include <cstdlib>

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
    if (SUCCEEDED(GetRegisters()))
    {
        std::cout << "Register Dump: \n";
        for (const auto& reg : registers_)
        {
            std::cout << "Reg = " << std::hex << reg.Reg64 << std::dec << "\n";
        }
    }
}

void VirtualProcessor::DetailedDumpRegisters()
{
    if (SUCCEEDED(GetRegisters()))
    {
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

HRESULT VirtualProcessor::ConfigureVM(const VMConfig& config)
{
    vmConfig_ = config;
    UINT32 cpuCount = config.cpuCount;
    auto result = WHvSetPartitionProperty(partitionHandle_, WHvPartitionPropertyCodeProcessorCount, &cpuCount, sizeof(cpuCount));
    if (FAILED(result))
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to set partition property (cpu count).");
        return result;
    }
    logger_.Log(Logger::LogLevel::Info, "Partition property (cpu count) set successfully." 
		+ std::to_string(result) + ", cpuCount = "
		+ std::to_string(cpuCount) + ", partitionHandle = "
		+ std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
    return result;

}

VirtualProcessor::VMConfig VirtualProcessor::GetVMConfig() const
{
    return vmConfig_;
}

UINT64 VirtualProcessor::GetCPUUsage()
{
    if (partitionHandle_ == nullptr)
    {
        logger_.Log(Logger::LogLevel::Error, "Partition handle is invalid. Cannot get CPU usage.");
        return 0;
    }

    UINT32 cpuUsage = 0;
    HRESULT result = WHvGetVirtualProcessorCounters(partitionHandle_, index_, WHV_PROCESSOR_COUNTER_SET(0), 0, cpuUsage, nullptr);

    if (FAILED(result))
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to get CPU usage: HRESULT "
            + std::to_string(result) + ", index = "
            + std::to_string(index_) + ", partitionHandle = "
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
        return 0;
    }

    logger_.Log(Logger::LogLevel::Info, "CPU usage retrieved successfully: "
        + std::to_string(cpuUsage) + ", index = "
        + std::to_string(index_) + ", partitionHandle = "
        + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));

    return cpuUsage;
}

UINT VirtualProcessor::GetActiveThreadCount()
{   
    if (partitionHandle_ == nullptr)
	{
		logger_.Log(Logger::LogLevel::Error, "Partition handle is invalid. Cannot get active thread count.");
		return 0;
	}

    UINT32 activeThreadCount = 0;
    HRESULT result = WHvGetVirtualProcessorCounters(partitionHandle_, index_, WHV_PROCESSOR_COUNTER_SET(1), 0, activeThreadCount, nullptr);

    if (FAILED(result))
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to get active thread count: HRESULT "
            + std::to_string(result) + ", index = "
            + std::to_string(index_) + ", partitionHandle = "
            + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));
        return 0;
    }

    logger_.Log(Logger::LogLevel::Info, "Active thread count retrieved successfully: "
        + std::to_string(activeThreadCount) + ", index = "
        + std::to_string(index_) + ", partitionHandle = "
        + std::to_string(reinterpret_cast<uintptr_t>(partitionHandle_)));

    return activeThreadCount;
}

bool VirtualProcessor::SetupKernelMemory()
{
    auto kernel = static_cast<Kernel*>(_aligned_malloc(sizeof(Kernel), 4096));
    if (!kernel)
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to allocate aligned memory for Kernel.");
        return false;
    }

    memset(kernel, 0, sizeof(Kernel));
    assert((reinterpret_cast<uintptr_t>(kernel) & (4096 - 1)) == 0);

    kernel->pml4[0] = (kernel_start + offsetof(Kernel, pdpt)) | (PTE_P | PTE_RW | PTE_US);
    kernel->pdpt[0] = 0x0 | (PTE_P | PTE_RW | PTE_US | PTE_PS);

    auto result = WHvMapGpaRange(partitionHandle_, kernel, kernel_start, sizeof(Kernel),
        WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagWrite);
    if (FAILED(result))
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to map kernel memory.");
        _aligned_free(kernel);
        return false;
    }

    logger_.Log(Logger::LogLevel::Info, "Kernel memory setup successfully." 
		+ std::to_string(result) + ", kernel = "
		+ std::to_string(reinterpret_cast<uintptr_t>(kernel)) + ", kernel_start = "
		+ std::to_string(kernel_start) + ", sizeof(Kernel) = "
		+ std::to_string(sizeof(Kernel)));

    _aligned_free(kernel);
    return true;
}


bool VirtualProcessor::MapUserSpace()
{
    void* user_page = _aligned_malloc(4096, 4096);
    if (!user_page)
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to allocate user space.");
        return false;
    }

    memcpy(user_page, user_code[vendor], sizeof(user_code[vendor]));

    auto result = WHvMapGpaRange(partitionHandle_, user_page, user_start, 4096,
        WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagExecute);
    if (FAILED(result))
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to map user space.");
        _aligned_free(user_page);
        return false;
    }

    logger_.Log(Logger::LogLevel::Info, "User space mapped successfully."
        + std::to_string(result) + ", user_page = "
		+ std::to_string(reinterpret_cast<uintptr_t>(user_page)) + ", user_start = "
		+ std::to_string(user_start) + ", 4096");

    _aligned_free(user_page);
    return true;
}


void VirtualProcessor::Run()
{
    if (partitionHandle_ == nullptr)
    {
        logger_.Log(Logger::LogLevel::Error, "Partition handle is invalid. Cannot run virtual processor.");
        return;
    }

    if (!SetupKernelMemory())
    {
        logger_.Log(Logger::LogLevel::Error, "Kernel memory setup failed.");
        return;
    }

    if (!MapUserSpace())
    {
        logger_.Log(Logger::LogLevel::Error, "User space mapping failed.");
        return;
    }

    SetRegisters();

    WHV_RUN_VP_EXIT_CONTEXT context;
    auto result = WHvRunVirtualProcessor(partitionHandle_, index_, &context, sizeof(context));

    if (SUCCEEDED(result))
    {
        logger_.Log(Logger::LogLevel::Info, "Virtual Processor is running.");

        UINT64 rip = GetSpecificRegister(WHvX64RegisterRip);
        logger_.Log(Logger::LogLevel::Info, "Instruction Pointer (RIP): " + std::to_string(rip));

        switch (context.ExitReason)
        {
        case WHvRunVpExitReasonHypercall:
            logger_.Log(Logger::LogLevel::Info, "The vmcall instruction executed.");
            //auto interruptController = InterruptController(partitionHandle_);
            break;
        case WHvRunVpExitReasonMemoryAccess:
        {
            // TODO: FIX THIS!!!
            const auto& memoryAccessContext = context.MemoryAccess;

            logger_.Log(Logger::LogLevel::Info, "Memory Access Exit Reason:");
            logger_.Log(Logger::LogLevel::Info, "Instruction Byte Count: " + std::to_string(memoryAccessContext.InstructionByteCount));
            logger_.Log(Logger::LogLevel::Info, "Guest Physical Address (GPA): " + std::to_string(memoryAccessContext.Gpa));
            logger_.Log(Logger::LogLevel::Info, "Guest Virtual Address (GVA): " + std::to_string(memoryAccessContext.Gva));

            if (memoryAccessContext.InstructionByteCount > 0)
            {
                std::string instructionBytes;
                for (size_t i = 0; i < memoryAccessContext.InstructionByteCount; ++i)
                {
                    instructionBytes += "0x" + std::to_string(memoryAccessContext.InstructionBytes[i]) + " ";
                }
                logger_.Log(Logger::LogLevel::Info, "Instruction Bytes: " + instructionBytes);
            }
            else
            {
                logger_.Log(Logger::LogLevel::Info, "No instruction bytes to log.");
            }
        }
        break;
        default:
            logger_.Log(Logger::LogLevel::Info, "Unhandled exit reason: " + std::to_string(context.ExitReason));
            break;
        }
    }
    else
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to run Virtual Processor.");
    }
}

