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

class HypervisorStateMachine
{
public:
    HypervisorStateMachine(size_t memorySize);
    ~HypervisorStateMachine();

    void Start();
    void Stop();
    void RunGUI();

    enum class State
    {
        Initializing,
        Ready,
        Running,
        Stopped,
        Error
    };

    void TransitionState(State newState);
    void CheckHypervisorCapability();
    void SetupPartition();
    void InitializeComponents();
    bool RunHypervisor();
    void RunGui();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();

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

    ID3D11Device* g_pd3dDevice;
    ID3D11DeviceContext* g_pd3dDeviceContext;
    IDXGISwapChain* g_pSwapChain;
    ID3D11RenderTargetView* g_mainRenderTargetView;
    IDXGIFactory* g_pDXGIFactory;

    HWND hwnd;

    void DisplayUsage();
};