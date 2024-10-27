#include "HypervisorStateMachine.h"
#include <iostream>

HypervisorStateMachine::HypervisorStateMachine(size_t memorySize)
    : memorySize_(memorySize), currentState_(State::Initializing), running_(false),
    virtualProcessor_(nullptr), gui_(nullptr), g_pd3dDevice(NULL),
    g_pd3dDeviceContext(NULL), g_pSwapChain(NULL), g_mainRenderTargetView(NULL), hwnd(NULL), 
    interruptController_(partition_.GetHandle()), snapshotManager_(partition_.GetHandle()), partition_(), memoryManager_(partition_.GetHandle(), memorySize_)
{

}

HypervisorStateMachine::~HypervisorStateMachine()
{
    Stop();
    CleanupDeviceD3D();
}

void HypervisorStateMachine::Start()
{
    CheckHypervisorCapability();
    SetupPartition();
    InitializeComponents();
    RunHypervisor();
}

void HypervisorStateMachine::Stop()
{
    running_ = false;
}

void HypervisorStateMachine::RunGUI(HWND hwnd)
{
    this->hwnd = hwnd;
    RunGui();
}

void HypervisorStateMachine::RunGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        return;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("MicroHypervisor GUI", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Memory Size: %zu bytes", memorySize_);

        {
            std::lock_guard<std::mutex> lock(outputMutex);
            ImGui::BeginChild("Output", ImVec2(0, 200), true);
            ImGui::TextWrapped("%s", outputBuffer.str().c_str());
            ImGui::EndChild();
        }

        if (ImGui::Button("Start Hypervisor"))
        {
            {
                std::lock_guard<std::mutex> lock(outputMutex);
                outputBuffer.str("");
                outputBuffer.clear();
                outputBuffer << "Starting hypervisor...\n";
            }

            ImGui::BeginDisabled();

            std::thread([=]() mutable {
                Start();
                }).detach();

            ImGui::EndDisabled();
        }
        ImGui::End();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void HypervisorStateMachine::CheckHypervisorCapability()
{
    WHV_CAPABILITY capability = {};
    HRESULT result = WHvGetCapability(WHvCapabilityCodeHypervisorPresent, &capability, sizeof(capability), nullptr);
    if (result != S_OK || !capability.HypervisorPresent)
    {
        TransitionState(State::Error);
        outputBuffer << "[Error]: Hypervisor platform not available.\n";
        return;
    }
    TransitionState(State::Ready);
}

void HypervisorStateMachine::SetupPartition()
{
    if (!partition_.Setup())
    {
        TransitionState(State::Error);
        outputBuffer << "[Error]: Failed to set up the partition.\n";
        return;
    }

    if (!partition_.CreateVirtualProcessor(0))
    {
        TransitionState(State::Error);
        outputBuffer << "[Error]: Failed to create virtual processor.\n";
        return;
    }
    outputBuffer << "[Info]: Virtual processor created successfully.\n";

    virtualProcessor_ = new VirtualProcessor(partition_.GetHandle(), 0);
    if (virtualProcessor_ == nullptr)
    {
        TransitionState(State::Error);
        outputBuffer << "[Error]: Failed to create VirtualProcessor instance.\n";
        return;
    }
    outputBuffer << "[Info]: VirtualProcessor instance created successfully.\n";

    if (!interruptController_.Setup())
    {
        TransitionState(State::Error);
        outputBuffer << "[Error]: Failed to initialize Interrupt Controller.\n";
        return;
    }

    if (!memoryManager_.Initialize())
    {
        TransitionState(State::Error);
        outputBuffer << "[Error]: Failed to initialize MemoryManager.\n";
        return;
    }

    TransitionState(State::Ready);
}

void HypervisorStateMachine::InitializeComponents()
{
    if (!interruptController_.Setup())
    {
        TransitionState(State::Error);
        outputBuffer << "[Error]: Failed to initialize InterruptController.\n";
        return;
    }
    if (!memoryManager_.Initialize())
    {
        TransitionState(State::Error);
        outputBuffer << "[Error]: Failed to initialize MemoryManager.\n";
        return;
    }
    if (!snapshotManager_.Initialize())
    {
        TransitionState(State::Error);
        outputBuffer << "[Error]: Failed to initialize SnapshotManager.\n";
        return;
    }
    TransitionState(State::Running);
}

bool HypervisorStateMachine::RunHypervisor()
{
    running_ = true;
    while (running_)
    {
        HRESULT hr = virtualProcessor_->SaveState();
        if (FAILED(hr))
        {
            TransitionState(State::Error);
            outputBuffer << "[Error]: Failed to save the state of the virtual processor.\n";
            return false;
        }

        // TODO: ADD simulate hypervisor running
        virtualProcessor_->DumpRegisters();
        virtualProcessor_->DetailedDumpRegisters();

        hr = virtualProcessor_->RestoreState();
        if (FAILED(hr))
        {
            TransitionState(State::Error);
            outputBuffer << "[Error]: Failed to restore the state of the virtual processor.\n";
            return false;
        }

        // ad any additional operations
    }
    TransitionState(State::Stopped);
    return true;
}

void HypervisorStateMachine::PrintOutputBuffer()
{
    std::string output;
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        output = outputBuffer.str();
        outputBuffer.str("");
        outputBuffer.clear();
    }

    if (!output.empty())
    {
        std::cout << output;
    }
}

void HypervisorStateMachine::TransitionState(State newState)
{
    std::string stateMessage;
    switch (newState)
    {
    case State::Initializing:
        stateMessage = "[State]: Initializing\n";
        break;
    case State::Ready:
        stateMessage = "[State]: Ready\n";
        break;
    case State::Running:
        stateMessage = "[State]: Running\n";
        break;
    case State::Stopped:
        stateMessage = "[State]: Stopped\n";
        break;
    case State::Error:
        stateMessage = "[State]: Error occurred\n";
        break;
    }

    {
        std::lock_guard<std::mutex> lock(outputMutex);
        outputBuffer << stateMessage;
    }

    PrintOutputBuffer();
}
void HypervisorStateMachine::DisplayUsage()
{
    std::cout << "Usage: MicroHypervisor [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -m, --memory <size>   Set the memory size in bytes (default: 4194304)\n";
    std::cout << "  --gui                 Launch GUI mode\n";
    std::cout << "  -h, --help            Show this help message\n";
}

bool HypervisorStateMachine::CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    if (!hWnd)
    {
        std::cerr << "[Error]: HWND is NULL in CreateDeviceD3D." << std::endl;
        return false;
    }

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (FAILED(res))
    {
        std::cerr << "[Error]: D3D11CreateDeviceAndSwapChain failed with HRESULT: 0x" << std::hex << res << std::endl;
        return false;
    }
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void HypervisorStateMachine::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void HypervisorStateMachine::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void HypervisorStateMachine::CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}
