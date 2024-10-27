#include "Emulator.h"
#include <iostream>

static LONG __stdcall EIoPortCallback(void* Context, WHV_EMULATOR_IO_ACCESS_INFO* IoAccess)
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

static LONG __stdcall EMemoryCallback(void* Context, WHV_EMULATOR_MEMORY_ACCESS_INFO* MemoryAccess)
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

static LONG __stdcall EGetVirtualProcessorRegistersCallback(void* Context, const WHV_REGISTER_NAME* RegisterNames, UINT32 RegisterCount, WHV_REGISTER_VALUE* RegisterValues)
{
    for (UINT32 i = 0; i < RegisterCount; ++i)
    {
        RegisterValues[i].Reg64 = 0xDEADBEEF;
        std::cout << "Register " << RegisterNames[i] << " read value: " << std::hex << RegisterValues[i].Reg64 << std::dec << std::endl;
    }
    return S_OK;
}

static LONG __stdcall ESetVirtualProcessorRegistersCallback(void* Context, const WHV_REGISTER_NAME* RegisterNames, UINT32 RegisterCount, const WHV_REGISTER_VALUE* RegisterValues)
{
    for (UINT32 i = 0; i < RegisterCount; ++i)
    {
        std::cout << "Register " << RegisterNames[i] << " set value: " << std::hex << RegisterValues[i].Reg64 << std::dec << std::endl;
    }
    return S_OK;
}

static LONG __stdcall ETranslateGvaPageCallback(void* Context, WHV_GUEST_VIRTUAL_ADDRESS Gva, WHV_TRANSLATE_GVA_FLAGS TranslateFlags, WHV_TRANSLATE_GVA_RESULT_CODE* TranslationResult, UINT64* Gpa)
{
    *Gpa = Gva;
    *TranslationResult = WHvTranslateGvaResultSuccess;
    std::cout << "Translated GVA: 0x" << std::hex << Gva << " to GPA: 0x" << std::hex << *Gpa << std::dec << std::endl;
    return S_OK;
}

Emulator::Emulator() : handle_(nullptr), logger_("Emulator.log")
{
    ZeroMemory(&callbacks_, sizeof(callbacks_));
    callbacks_.Size = sizeof(WHV_EMULATOR_CALLBACKS);
    callbacks_.Reserved = 0;

    callbacks_.WHvEmulatorIoPortCallback = EIoPortCallback;
    callbacks_.WHvEmulatorMemoryCallback = EMemoryCallback;
    callbacks_.WHvEmulatorGetVirtualProcessorRegisters = EGetVirtualProcessorRegistersCallback;
    callbacks_.WHvEmulatorSetVirtualProcessorRegisters = ESetVirtualProcessorRegistersCallback;
    callbacks_.WHvEmulatorTranslateGvaPage = ETranslateGvaPageCallback;
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
        logger_.Log(Logger::LogLevel::Error, "Emulator initialization failed with HRESULT: " + std::to_string(result));
        switch (result)
        {
        case E_INVALIDARG:
            logger_.Log(Logger::LogLevel::Error, "Invalid argument passed to WHvEmulatorCreateEmulator.");
            break;
        case E_NOTIMPL:
            logger_.Log(Logger::LogLevel::Error, "Operation not supported. Ensure that your system meets the requirements.");
            break;
        case E_OUTOFMEMORY:
            logger_.Log(Logger::LogLevel::Error, "Insufficient resources to create the emulator. Consider freeing up resources.");
            break;
        default:
            logger_.Log(Logger::LogLevel::Error, "An unknown error occurred. HRESULT: " + std::to_string(result));
            break;
        }
        return false;
    }
    return true;
}
