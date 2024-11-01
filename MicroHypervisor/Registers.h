#ifndef REGISTERS_H
#define REGISTERS_H

#include <Windows.h>
#include <WinHvEmulation.h>
#include <cstdint>

/**
 * @brief 
 * 
 */
constexpr WHV_REGISTER_NAME regNames[] = {
    WHvX64RegisterRax, WHvX64RegisterRcx, WHvX64RegisterRdx, WHvX64RegisterRbx,
    WHvX64RegisterRsp, WHvX64RegisterRbp, WHvX64RegisterRsi, WHvX64RegisterRdi,
    WHvX64RegisterR8,  WHvX64RegisterR9,  WHvX64RegisterR10, WHvX64RegisterR11,
    WHvX64RegisterR12, WHvX64RegisterR13, WHvX64RegisterR14, WHvX64RegisterR15,
    WHvX64RegisterRip, WHvX64RegisterRflags,
    WHvX64RegisterEs,  WHvX64RegisterCs,  WHvX64RegisterSs,  WHvX64RegisterDs,
    WHvX64RegisterFs,  WHvX64RegisterGs,
    WHvX64RegisterGdtr,
    WHvX64RegisterCr0, WHvX64RegisterCr2, WHvX64RegisterCr3, WHvX64RegisterCr4,
    WHvX64RegisterCr8,
    WHvX64RegisterEfer, WHvX64RegisterLstar, WHvRegisterPendingInterruption,
};

/// @namespace CR0 for x86 Control Register 0 \class CR0
namespace CR0
{
    constexpr uint32_t PE = 1u;                     // Protected Mode Enable
    constexpr uint32_t MP = (1u << 1);              // Monitor co-processor
    constexpr uint32_t EM = (1u << 2);              // Emulation
    constexpr uint32_t TS = (1u << 3);              // Task switched
    constexpr uint32_t ET = (1u << 4);              // Extension type
    constexpr uint32_t NE = (1u << 5);              // Numeric error
    constexpr uint32_t WP = (1u << 16);             // Write protect
    constexpr uint32_t AM = (1u << 18);             // Alignment mask
    constexpr uint32_t NW = (1u << 29);             // Not-write through
    constexpr uint32_t CD = (1u << 30);             // Cache disable
    constexpr uint32_t PG = (1u << 31);             // Paging
}

/// @namespace CR4 for x86 Control Register 4 \class CR4
namespace CR4
{
    constexpr uint32_t VME = 1u;                    // Virtual 8086 Mode Extensions
    constexpr uint32_t PVI = (1u << 1);             // Protected-mode Virtual Interrupts
    constexpr uint32_t TSD = (1u << 2);             // Time Stamp Disable
    constexpr uint32_t DE = (1u << 3);              // Debugging Extensions
    constexpr uint32_t PSE = (1u << 4);             // Page Size Extension
    constexpr uint32_t PAE = (1u << 5);             // Physical Address Extension
    constexpr uint32_t MCE = (1u << 6);             // Machine Check Exception
    constexpr uint32_t PGE = (1u << 7);             // Page Global Enabled
    constexpr uint32_t PCE = (1u << 8);             // Performance-Monitoring Counter enable
    constexpr uint32_t OSFXSR = (1u << 9);          // OS support for FXSAVE/FXRSTOR
    constexpr uint32_t OSXMMEXCPT = (1u << 10);     // SIMD Floating-Point Exceptions
    constexpr uint32_t UMIP = (1u << 11);           // User-Mode Instruction Prevention
    constexpr uint32_t VMXE = (1u << 13);           // Virtual Machine Extensions Enable
    constexpr uint32_t SMXE = (1u << 14);           // Safer Mode Extensions Enable
    constexpr uint32_t FSGSBASE = (1u << 16);       // Enables RDFSBASE, RDGSBASE, etc.
    constexpr uint32_t PCIDE = (1u << 17);          // PCID Enable
    constexpr uint32_t OSXSAVE = (1u << 18);        // XSAVE/Processor Extended States Enable
    constexpr uint32_t SMEP = (1u << 20);           // SMEP Enable
    constexpr uint32_t SMAP = (1u << 21);           // SMAP Enable
    constexpr uint32_t PKE = (1u << 22);            // Protection Key Enable
}

/// @namespace EFER for x86 Extended Feature Enable Register \class EFER
namespace EFER
{
    constexpr uint32_t SCE = 1u;                    // System Call Extensions
    constexpr uint32_t LME = (1u << 8);             // Long Mode Enable
    constexpr uint32_t LMA = (1u << 10);            // Long Mode Active
    constexpr uint32_t NXE = (1u << 11);            // No-Execute Enable
    constexpr uint32_t SVME = (1u << 12);           // Secure Virtual Machine Enable
    constexpr uint32_t LMSLE = (1u << 13);          // Long Mode Segment Limit Enable
    constexpr uint32_t FFXSR = (1u << 14);          // Fast FXSAVE/FXRSTOR
    constexpr uint32_t TCE = (1u << 15);            // Translation Cache Extension
}

/// @namespace PML4 for x86 Page Map Level 4 \class PML4
namespace PDE64
{
    constexpr uint32_t PRESENT = 1u;
    constexpr uint32_t RW = (1u << 1);
    constexpr uint32_t USER = (1u << 2);
    constexpr uint32_t ACCESSED = (1u << 5);
    constexpr uint32_t DIRTY = (1u << 6);
    constexpr uint32_t PS = (1u << 7);
    constexpr uint32_t G = (1u << 8);
}

/// @namespace ProcCounters
namespace ProcCounters
{
    struct WHV_VIRTUAL_PROCESSOR_COUNTERS
    {
        UINT64 TotalRuntime100ns;
        UINT64 TotalWaitTime100ns;
        UINT64 TotalDispatchCount;
        UINT64 TotalInterrupts;
        UINT64 TotalExceptions;
        UINT64 TotalIoInstructions;
        UINT64 TotalMmioInstructions;
        UINT64 TotalIoPortInstructions;
        UINT64 TotalIoPortAccesses;
        UINT64 TotalMemoryInstructions;
        UINT64 TotalMemoryAccesses;
        UINT64 TotalMemoryReads;
        UINT64 TotalMemoryWrites;
    };
}

#endif // REGISTERS_H
