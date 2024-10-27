#include "VirtualProcessor.h"
#include <iostream>
#include <iomanip>

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
    return WHvGetVirtualProcessorRegisters(partitionHandle_, index_, regNames, std::size(regNames), registers_.data());
}

HRESULT VirtualProcessor::SetRegisters()
{
    return WHvSetVirtualProcessorRegisters(partitionHandle_, index_, regNames, std::size(regNames), registers_.data());
}

HRESULT VirtualProcessor::SetSpecificRegister(WHV_REGISTER_NAME regName, UINT64 value)
{
    for (size_t i = 0; i < std::size(regNames); ++i)
    {
        if (regNames[i] == regName)
        {
            registers_[i].Reg64 = value;
            return S_OK;
        }
    }
    return E_INVALIDARG;
}

UINT64 VirtualProcessor::GetSpecificRegister(WHV_REGISTER_NAME regName) const
{
    for (size_t i = 0; i < std::size(regNames); ++i)
    {
        if (regNames[i] == regName)
        {
            return registers_[i].Reg64;
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
    return S_OK;
}

HRESULT VirtualProcessor::RestoreState()
{
    if (!isRunning_) return E_FAIL;

    registers_ = savedRegisters_;
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

void VirtualProcessor::Run()
{
    registers_[WHvX64RegisterRip].Reg64 = 0x1000;
    SetRegisters();
    WHvRunVirtualProcessor(partitionHandle_, index_, 0, 0);

    if (GetSpecificRegister(WHvX64RegisterRip) == 0x1000)
	{
        logger_.Log(Logger::LogLevel::Info, "Virtual Processor is running.");
	}
	else
	{
        logger_.Log(Logger::LogLevel::Error, "Virtual Processor failed to run.");
	}
}