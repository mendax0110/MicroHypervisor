#include "InterruptController.h"
#include "Registers.h"
#include <iostream>

InterruptController::InterruptController(WHV_PARTITION_HANDLE partitionHandle)
    : partitionHandle_(partitionHandle) {}


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
        std::cerr << "[Error]: Failed to setup interrupt controller: HRESULT " << std::hex << result << std::dec << "\n";
        return false;
    }
    return true;
}

void InterruptController::InjectInterrupt(UINT32 interruptVector) const
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
        std::cerr << "[Error]: Failed to inject interrupt: HRESULT " << std::hex << result << std::dec << "\n";
    }
    else
    {
        std::cout << "Injected interrupt vector: " << interruptVector << "\n";
    }
}
