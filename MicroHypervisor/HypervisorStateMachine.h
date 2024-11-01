#ifndef HYPERVISOR_STATE_MACHINE_H
#define HYPERVISOR_STATE_MACHINE_H

#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <sstream>
#include <fstream>
#include <Windows.h>
#include <memory>
#include <d3d11.h>
#include <dxgi.h> 
#include <tchar.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include "Partition.h"
#include "VirtualProcessor.h"
#include "Emulator.h"
#include "InterruptController.h"
#include "MemoryManager.h"
#include "SnapshotManager.h"
#include "Timer.h"
#include "Logger.h"

class HypervisorGUI;

/// @brief Hypervisor State Machine class for the Hypervisor \class HypervisorStateMachine
class HypervisorStateMachine
{
public:
    HypervisorStateMachine(size_t memorySize);
    ~HypervisorStateMachine();

    /**
     * @brief Function to start the Hypervisor
     * 
     */
    void Start();

    /**
     * @brief Function to stop the Hypervisor
     * 
     */
    void Stop();

    /**
     * @brief Enum with the possible states of the Hypervisor
     * 
     */
    enum class State
    {
        Initializing,
        Ready,
        Running,
        Stopped,
        Error
    };

    /**
	 * @brief Enum with the possible menu options
	 * 
	 */
    enum class MenuOption
    {
        Continue,
        Restart,
        Stop,
        Start,
        SaveSnapshot,
        RestoreSnapshot,
        DumpRegisters,
        DetailedDumpRegisters,
        SetRegisters,
        GetRegisters,
        SetSpecificRegister,
        GetSpecificRegister,
        ConfigureVM,
        GetVMConfig,
    };

    /**
     * @brief Mapping of commands
     *
     */
    std::unordered_map<std::string, MenuOption> commandMap = {
        {"start", MenuOption::Start},
        {"stop", MenuOption::Stop},
        {"restart", MenuOption::Restart},
        {"save snapshot", MenuOption::SaveSnapshot},
        {"restore snapshot", MenuOption::RestoreSnapshot},
        {"dump registers", MenuOption::DumpRegisters},
        {"set registers", MenuOption::SetRegisters},
        {"get registers", MenuOption::GetRegisters},
        {"set memory size", MenuOption::Continue}
    };

    /**
	 * @brief Function to show the menu
	 * 
	 * @return MenuOption -> The selected menu option
	 */
    MenuOption ShowMenu();

    /**
	 * @brief Function to check for an interrupt
	 * 
	 * @return true -> if an interrupt is detected
	 * @return false -> if no interrupt is detected
	 */
    bool CheckForInterrupt();

    /**
     * @brief Transition the state of the Hypervisor, from one state to another
     * 
     * @param newState -> State, the new state of the Hypervisor
     */
    void TransitionState(State newState);

    /**
     * @brief Checks if the Hypervisor has the required capabilities
     * 
     */
    void CheckHypervisorCapability();

    /**
     * @brief Function to setup the partition
     * 
     */
    void SetupPartition();

    /**
     * @brief Function to initialize the components of the Hypervisor
     * 
     */
    void InitializeComponents();

    /**
     * @brief Main loop of the Hypervisor, runs the Hypervisor
     * 
     * @return true -> if the Hypervisor runs successfully
     * @return false -> if the Hypervisor fails to run
     */
    bool RunHypervisor();

    /**
     * @brief Abstract function to run the Hypervisor via Gui
     * 
     */
    void RunGui();

    /**
     * @brief Abstract function to run the Hypervisor via Cli
     *
     */
    void RunCli();

    /**
     * @brief Adapt memory size
     *
     */
    void UpdateMemorySize();

    /**
     * @brief Function to diplay cli menu
     *
     */
    void DisplayUsageAndMenu();

    /**
     * @brief Create a Render Target object
     * 
     */
    void CreateRenderTarget();

    /**
     * @brief Cleans the Render Target object
     * 
     */
    void CleanupRenderTarget();

    /**
     * @brief Create a Device D 3 D object
     * 
     * @param hWnd -> HWND, Handle to the Window
     * @return true -> if the Device D3D is created successfully
     * @return false -> if the Device D3D creation fails
     */
    bool CreateDeviceD3D(HWND hWnd);

    /**
     * @brief Cleans the Device D3D object
     * 
     */
    void CleanupDeviceD3D();

    /**
     * @brief Function to print the output buffer
	 * 
	 */
    void PrintOutputBuffer();

    /**
     * @brief Function to check if gui mode is enabled
     *
     */
    bool IsGuiMode() const;

    /**
     * @brief Function to parse the arguments
     * 
     * @param argc -> int, Number of arguments
     * @param argv -> char**, Arguments
     */
    bool ParseArguments(int argc, char* argv[]);

    /**
	 * @brief Function to get the memory size of the Hypervisor
	 * 
	 * @return size_t -> Memory size of the Hypervisor
	 */
    size_t GetMemorySize() const;

    /**
     * @brief Function to handle the menu option
     *
     * @param option -> MenuOption, The selected menu option
     */
    void HandleMenuOption(MenuOption option);

    IDXGISwapChain* g_pSwapChain;
    ID3D11Device* g_pd3dDevice;

private:
    size_t memorySize_;
    State currentState_;
    std::atomic<bool> running_;
    std::mutex outputMutex;
    std::mutex stateMutex;
    std::stringstream outputBuffer;

    Partition partition_;
    VirtualProcessor* virtualProcessor_;
    Emulator emulator_;
    InterruptController interruptController_;
    MemoryManager memoryManager_;
    SnapshotManager snapshotManager_;
    Logger logger_;

    HypervisorGUI* gui_;

    ID3D11DeviceContext* g_pd3dDeviceContext;
    ID3D11RenderTargetView* g_mainRenderTargetView;
    IDXGIFactory* g_pDXGIFactory;

    HWND hwnd;

    bool guiMode_ = false;
    volatile bool pendingInterrupt_ = false;
};

#endif // HYPERVISOR_STATE_MACHINE_H