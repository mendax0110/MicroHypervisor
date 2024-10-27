#include "InterruptController.h"
#include "Registers.h"
#include <iostream>

InterruptController::InterruptController(WHV_PARTITION_HANDLE partitionHandle)
    : partitionHandle_(partitionHandle), logger_("InterruptController.log")
{

}

InterruptController::~InterruptController() {}

bool InterruptController::Setup()
{
    std::vector<WHV_REGISTER_NAME> interruptRegNames = {
        WHV_REGISTER_NAME::WHvRegisterPendingInterruption,
        WHV_REGISTER_NAME::WHvRegisterInterruptState
    };

    interruptRegisters_.resize(interruptRegNames.size());

    HRESULT result = WHvGetVirtualProcessorRegisters(
        partitionHandle_, 0, interruptRegNames.data(), static_cast<UINT32>(interruptRegNames.size()), interruptRegisters_.data()
    );

    if (FAILED(result))
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to setup interrupt controller: HRESULT " + std::to_string(result));
        return false;
    }
    return true;
}

void InterruptController::InjectInterrupt(UINT32 interruptVector)
{
    WHV_INTERRUPT_CONTROL interruptControl = {};
    interruptControl.Type = WHvX64PendingInterrupt;
    interruptControl.Vector = interruptVector;
    interruptControl.Reserved = 0;

    auto result = WHvSetVirtualProcessorInterruptControllerState(
        partitionHandle_, 0, &interruptControl, sizeof(interruptControl)
    );

    if (FAILED(result))
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to inject interrupt: HRESULT " + std::to_string(result));
    }
    else
    {
        logger_.Log(Logger::LogLevel::Info, "Injected interrupt vector: " + std::to_string(interruptVector));
    }
}
