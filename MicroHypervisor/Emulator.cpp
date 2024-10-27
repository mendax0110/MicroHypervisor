#include "Emulator.h"
#include <WinHvEmulation.h>
#include <iostream>

LONG __stdcall IoPortCallback(void* Context, WHV_EMULATOR_IO_ACCESS_INFO* IoAccess)
{
    if (IoAccess->Direction == 0)
    {
        if (IoAccess->Port == 0x60)
        {
            IoAccess->Data = 0xFF;
            std::cout << "[SUCCESS]: Read from I/O port 0x" << std::hex << IoAccess->Port
                << ": " << std::hex << IoAccess->Data << std::dec << std::endl;
        }
        else
        {
            std::cerr << "[ERROR]: Read from unsupported I/O port: 0x" << std::hex << IoAccess->Port << std::dec << std::endl;
            return E_INVALIDARG;
        }
    }
    else
    {
        if (IoAccess->Port == 0x60)
        {
            std::cout << "[SUCCESS]: Wrote to I/O port 0x" << std::hex << IoAccess->Port
                << ": " << std::hex << IoAccess->Data << std::dec << std::endl;
        }
        else
        {
            std::cerr << "[ERROR]: Write to unsupported I/O port: 0x" << std::hex << IoAccess->Port << std::dec << std::endl;
            return E_INVALIDARG;
        }
    }
    return S_OK;
}

LONG __stdcall MemoryCallback(void* Context, WHV_EMULATOR_MEMORY_ACCESS_INFO* MemoryAccess)
{
    if (MemoryAccess->Direction == 0)
    {
        if (MemoryAccess->GpaAddress == 0x1000)
        {
            MemoryAccess->Data[0] = 0xAB;
            std::cout << "[SUCCESS]: Read from memory address 0x" << std::hex << MemoryAccess->GpaAddress
                << ": " << std::hex << (int)MemoryAccess->Data[0] << std::dec << std::endl;
        }
        else
        {
            std::cerr << "[ERROR]: Read from unsupported memory address: 0x"
                << std::hex << MemoryAccess->GpaAddress << std::dec << std::endl;
            return E_INVALIDARG;
        }
    }
    else
    {
        if (MemoryAccess->GpaAddress == 0x1000)
        {
            std::cout << "[SUCCESS]: Wrote to memory address 0x" << std::hex << MemoryAccess->GpaAddress
                << ": " << std::hex << (int)MemoryAccess->Data[0] << std::dec << std::endl;
        }
        else
        {
            std::cerr << "[ERROR]: Write to unsupported memory address: 0x"
                << std::hex << MemoryAccess->GpaAddress << std::dec << std::endl;
            return E_INVALIDARG;
        }
    }
    return S_OK;
}

LONG __stdcall GetVirtualProcessorRegistersCallback(void* Context, const WHV_REGISTER_NAME* RegisterNames, UINT32 RegisterCount, WHV_REGISTER_VALUE* RegisterValues)
{
    for (UINT32 i = 0; i < RegisterCount; ++i)
    {
        RegisterValues[i].Reg64 = 0xDEADBEEF;
        std::cout << "Register " << RegisterNames[i] << " read value: " << std::hex << RegisterValues[i].Reg64 << std::dec << std::endl;
    }
    return S_OK;
}

LONG __stdcall SetVirtualProcessorRegistersCallback(void* Context, const WHV_REGISTER_NAME* RegisterNames, UINT32 RegisterCount, const WHV_REGISTER_VALUE* RegisterValues)
{
    for (UINT32 i = 0; i < RegisterCount; ++i)
    {
        std::cout << "Register " << RegisterNames[i] << " set value: " << std::hex << RegisterValues[i].Reg64 << std::dec << std::endl;
    }
    return S_OK;
}

LONG __stdcall TranslateGvaPageCallback(void* Context, WHV_GUEST_VIRTUAL_ADDRESS Gva, WHV_TRANSLATE_GVA_FLAGS TranslateFlags, WHV_TRANSLATE_GVA_RESULT_CODE* TranslationResult, UINT64* Gpa)
{
    *Gpa = Gva;
    *TranslationResult = WHvTranslateGvaResultSuccess;
    std::cout << "Translated GVA: 0x" << std::hex << Gva << " to GPA: 0x" << std::hex << *Gpa << std::dec << std::endl;
    return S_OK;
}

Emulator::Emulator() : handle_(nullptr)
{
    ZeroMemory(&callbacks_, sizeof(callbacks_));
    callbacks_.Size = sizeof(WHV_EMULATOR_CALLBACKS);
    callbacks_.Reserved = 0;

    callbacks_.WHvEmulatorIoPortCallback = IoPortCallback;
    callbacks_.WHvEmulatorMemoryCallback = MemoryCallback;
    callbacks_.WHvEmulatorGetVirtualProcessorRegisters = GetVirtualProcessorRegistersCallback;
    callbacks_.WHvEmulatorSetVirtualProcessorRegisters = SetVirtualProcessorRegistersCallback;
    callbacks_.WHvEmulatorTranslateGvaPage = TranslateGvaPageCallback;
}

Emulator::~Emulator()
{
    if (handle_)
    {
        WHvEmulatorDestroyEmulator(&handle_);
    }
}

bool Emulator::Initialize()
{
    HRESULT result = WHvEmulatorCreateEmulator(&callbacks_, &handle_);

    if (result != S_OK)
    {
        std::cerr << "[ERROR]: Emulator initialization failed with HRESULT: " << std::hex << result << std::dec << "\n";
        switch (result)
        {
        case E_INVALIDARG:
            std::cerr << "[ERROR]: Invalid argument passed to WHvEmulatorCreateEmulator.\n";
            break;
        case E_NOTIMPL:
            std::cerr << "[ERROR]: Operation not supported. Ensure that your system meets the requirements.\n";
            break;
        case E_OUTOFMEMORY:
            std::cerr << "[ERROR]: Insufficient resources to create the emulator. Consider freeing up resources.\n";
            break;
        default:
            std::cerr << "[ERROR]: An unknown error occurred. HRESULT: " << std::hex << result << "\n";
            break;
        }
        return false;
    }
    return true;
}
