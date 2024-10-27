#pragma once
#define _WIN32_WINNT 0x0A00
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


class HypervisorGUI;

/// @brief Hypervisor State Machine class for the Hypervisor \class HypervisorStateMachine
class HypervisorStateMachine
{
public:
    HypervisorStateMachine(size_t memorySize);
    ~HypervisorStateMachine();

    /**
     * @brief 
     * 
     */
    void Start();

    /**
     * @brief 
     * 
     */
    void Stop();

    /**
     * @brief 
     * 
     * @param hwnd 
     */
    void RunGUI(HWND hwnd);

    /**
     * @brief 
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
     * @brief 
     * 
     * @param newState 
     */
    void TransitionState(State newState);

    /**
     * @brief 
     * 
     */
    void CheckHypervisorCapability();

    /**
     * @brief 
     * 
     */
    void SetupPartition();

    /**
     * @brief 
     * 
     */
    void InitializeComponents();

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool RunHypervisor();

    /**
     * @brief 
     * 
     */
    void RunGui();

    /**
     * @brief Create a Render Target object
     * 
     */
    void CreateRenderTarget();

    /**
     * @brief 
     * 
     */
    void CleanupRenderTarget();

    /**
     * @brief Create a Device D 3 D object
     * 
     * @param hWnd 
     * @return true 
     * @return false 
     */
    bool CreateDeviceD3D(HWND hWnd);

    /**
     * @brief 
     * 
     */
    void CleanupDeviceD3D();

    /**
     * @brief 
     * 
     */
    void DisplayUsage();

    void PrintOutputBuffer();

    IDXGISwapChain* g_pSwapChain;
    ID3D11Device* g_pd3dDevice;

private:
    size_t memorySize_;
    State currentState_;
    std::atomic<bool> running_;
    std::mutex outputMutex;
    std::stringstream outputBuffer;

    Partition partition_;
    VirtualProcessor* virtualProcessor_;
    Emulator emulator_;
    InterruptController interruptController_;
    MemoryManager memoryManager_;
    SnapshotManager snapshotManager_;

    HypervisorGUI* gui_;

    ID3D11DeviceContext* g_pd3dDeviceContext;
    ID3D11RenderTargetView* g_mainRenderTargetView;
    IDXGIFactory* g_pDXGIFactory;

    HWND hwnd;
};