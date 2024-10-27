#ifndef INTERRUPT_CONTROLLER_H
#define INTERRUPT_CONTROLLER_H

#include <Windows.h>
#include <WinHvPlatform.h>
#include <WinHvPlatformDefs.h>
#include <vector>
#include "Logger.h"

/// @brief Interrupt Controller class for the Hypervisor \class InterruptController
class InterruptController
{
public:
    InterruptController(WHV_PARTITION_HANDLE partitionHandle);
    ~InterruptController();

    /**
     * @brief Setup of the Interrupt Controller
     *
     * @return true -> if the setup is successful
     * @return false -> if the setup fails
     */
    bool Setup();

    /**
     * @brief Injects a interrupt into the partition
     *
     * @param interruptVector -> UINT32, Interrupt Vector to inject
     */
    void InjectInterrupt(UINT32 interruptVector);

private:
    WHV_PARTITION_HANDLE partitionHandle_;
    std::vector<WHV_REGISTER_VALUE> interruptRegisters_;
    Logger logger_;
};

#endif // INTERRUPT_CONTROLLER_H