#include "HypervisorStateMachine.h"
#include <iostream>
#include <sstream>
#include <string>
#include <Windows.h>
#include <tchar.h>

int main(int argc, char* argv[])
{
    size_t memorySize = 0x400000; // Default memory size: 4MB

    HypervisorStateMachine hypervisor(memorySize);

    if (!hypervisor.ParseArguments(argc, argv))
    {
        return EXIT_FAILURE;
    }

    if (hypervisor.IsGuiMode())
    {
        hypervisor.RunGui();
    }
    else
    {
        hypervisor.Start();
    }

    return EXIT_SUCCESS;
}