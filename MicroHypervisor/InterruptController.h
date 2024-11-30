#ifndef INTERRUPT_CONTROLLER_H
#define INTERRUPT_CONTROLLER_H

#include <Windows.h>
#include <WinHvPlatform.h>
#include <WinHvPlatformDefs.h>
#include <vector>
#include <map>
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


    /**
     * @brief Structure to hold the Interrupt Information
     */
    typedef struct InterruptInfo
    {
        UINT32 interruptVector;
        WHV_INTERRUPT_TYPE interruptType;
        WHV_INTERRUPT_DESTINATION_MODE destinationMode;
        UINT32 destination;
        UINT32 reserved;
    } InterruptInfo;

    std::map<UINT32, std::string> interruptTypeMap =
    {
    };

    /**
     * @brief Function to observe the interrupt
     * 
     * @param interruptInfo -> struct with the interrupt information
     * 
     * @return -> true if the interrupt is observed, false otherwise
     */
    bool InterruptObserver(InterruptInfo& interruptInfo);

private:
    WHV_PARTITION_HANDLE partitionHandle_;
    std::vector<WHV_REGISTER_VALUE> interruptRegisters_;
    Logger logger_;
};

#endif // INTERRUPT_CONTROLLER_H