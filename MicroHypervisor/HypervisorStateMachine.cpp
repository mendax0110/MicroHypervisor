#include "HypervisorStateMachine.h"
#include <iostream>
#include <limits>
#include <conio.h>

HypervisorStateMachine::HypervisorStateMachine(size_t memorySize)
    : memorySize_(memorySize), currentState_(State::Initializing), running_(false),
    virtualProcessor_(nullptr), gui_(nullptr), g_pd3dDevice(NULL), g_pDXGIFactory(NULL),
    g_pd3dDeviceContext(NULL), g_pSwapChain(NULL), g_mainRenderTargetView(NULL), hwnd(NULL), 
    interruptController_(partition_.GetHandle()), snapshotManager_(partition_.GetHandle()), partition_(), 
    memoryManager_(partition_.GetHandle(), memorySize_), logger_("MicroHypervisor.log")
{
    logger_.Log(Logger::LogLevel::Info, "HypervisorStateMachine initialized.");
}

HypervisorStateMachine::~HypervisorStateMachine()
{
    logger_.Log(Logger::LogLevel::Info, "HypervisorStateMachine destroyed.");
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
    HRESULT result;

    result = WHvGetCapability(WHvCapabilityCodeHypervisorPresent, &capability, sizeof(capability), nullptr);
    if (result != S_OK || !capability.HypervisorPresent)
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Hypervisor platform not available.");
        logger_.LogStackTrace();
        return;
    }

    result = WHvGetCapability(WHvCapabilityCodeFeatures, &capability, sizeof(capability), nullptr);
    if (result != S_OK)
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to get hypervisor features capability.");
        logger_.LogStackTrace();
        return;
    }

    result = WHvGetCapability(WHvCapabilityCodeExtendedVmExits, &capability, sizeof(capability), nullptr);
    if (result != S_OK)
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to get extended VM exits capability.");
        logger_.LogStackTrace();
        return;
    }

    result = WHvGetCapability(WHvCapabilityCodeExceptionExitBitmap, &capability, sizeof(capability), nullptr);
    if (result != S_OK)
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to get exception exit bitmap capability.");
        logger_.LogStackTrace();
        return;
    }

    result = WHvGetCapability(WHvCapabilityCodeX64MsrExitBitmap, &capability, sizeof(capability), nullptr);
    if (result != S_OK)
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to get MSR exit bitmap capability.");
        logger_.LogStackTrace();
        return;
    }

    // GPA range populate flags
    //result = WHvGetCapability(WHvCapabilityCodeGpaRangePopulateFlags, &capability, sizeof(capability), nullptr);
    //if (result != S_OK)
    //{
    //    TransitionState(State::Error);
    //    outputBuffer << "[Error]: Failed to get GPA range populate flags capability.\n";
    //    return;
    //}

    //// sceduler features
    //result = WHvGetCapability(WHvCapabilityCodeSchedulerFeatures, &capability, sizeof(capability), nullptr);
    //if (result != S_OK)
    //{
    //    TransitionState(State::Error);
    //    outputBuffer << "[Error]: Failed to get scheduler features capability.\n";
    //    return;
    //}

    WHV_PROCESSOR_VENDOR vendor = {};
    result = WHvGetCapability(WHvCapabilityCodeProcessorVendor, &vendor, sizeof(vendor), nullptr);
    if (result == S_OK)
    {
        logger_.Log(Logger::LogLevel::Info, "Processor Vendor: " + std::to_string(vendor));
        logger_.LogStackTrace();
    }
    else
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to get processor vendor capability.");
        logger_.LogStackTrace();
        return;
    }

    WHV_PROCESSOR_FEATURES processorFeatures = {};
    result = WHvGetCapability(WHvCapabilityCodeProcessorFeatures, &processorFeatures, sizeof(processorFeatures), nullptr);
    if (result == S_OK)
    {
        logger_.Log(Logger::LogLevel::Info, "Processor Features: " + std::to_string(processorFeatures.IntelPrefetchSupport));
        logger_.LogStackTrace();
    }
    else
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to get processor features capability.");
        logger_.LogStackTrace();
        return;
    }

    TransitionState(State::Ready);
}

void HypervisorStateMachine::SetupPartition()
{
    if (!partition_.Setup())
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to set up the partition.");
        logger_.LogStackTrace();
        return;
    }

    if (!partition_.CreateVirtualProcessor(0))
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to create virtual processor.");
        logger_.LogStackTrace();
        return;
    }
    logger_.Log(Logger::LogLevel::Info, "Virtual processor created successfully.");
    logger_.LogStackTrace();

    virtualProcessor_ = new VirtualProcessor(partition_.GetHandle(), 0);
    if (virtualProcessor_ == nullptr)
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to create VirtualProcessor instance.");
        logger_.LogStackTrace();
        return;
    }
    logger_.Log(Logger::LogLevel::Info, "VirtualProcessor instance created successfully.");
    logger_.LogStackTrace();

    if (!interruptController_.Setup())
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to initialize Interrupt Controller.");
        logger_.LogStackTrace();
        return;
    }

    if (!memoryManager_.Initialize())
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to initialize MemoryManager.");
        logger_.LogStackTrace();
        return;
    }

    TransitionState(State::Ready);
}

void HypervisorStateMachine::InitializeComponents()
{
    if (!interruptController_.Setup())
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to initialize Interrupt Controller.");
        logger_.LogStackTrace();
        return;
    }
    if (!memoryManager_.Initialize())
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to initialize MemoryManager.");
        logger_.LogStackTrace();
        return;
    }
    if (!snapshotManager_.Initialize())
    {
        TransitionState(State::Error);
        logger_.Log(Logger::LogLevel::Error, "Failed to initialize SnapshotManager.");
        logger_.LogStackTrace();
        return;
    }
    TransitionState(State::Running);
}

bool HypervisorStateMachine::RunHypervisor()
{
    running_ = true;

    while (running_)
    {
        if (CheckForInterrupt())
        {
            MenuOption option = ShowMenu();
            switch (option)
            {
            case MenuOption::Continue:
                // Continue running the hypervisor
                break;
            case MenuOption::Restart:
                // Restart logic
                virtualProcessor_->RestoreState();
                break;
            case MenuOption::Stop:
                running_ = false;
                break;
            }
        }

        HRESULT hr = virtualProcessor_->SaveState();
        if (FAILED(hr))
        {
            TransitionState(State::Error);
            logger_.Log(Logger::LogLevel::Error, "Failed to save the state of the virtual processor.");
            return false;
        }

        // Simulate hypervisor running
        virtualProcessor_->Run();
        virtualProcessor_->DumpRegisters();
        virtualProcessor_->DetailedDumpRegisters();

        hr = virtualProcessor_->RestoreState();
        if (FAILED(hr))
        {
            TransitionState(State::Error);
            logger_.Log(Logger::LogLevel::Error, "Failed to restore the state of the virtual processor.");
            return false;
        }

        // Add any additional operations
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    TransitionState(State::Stopped);
    return true;
}

bool HypervisorStateMachine::CheckForInterrupt()
{
    if (_kbhit())
    {
        char ch = _getch();
        if (ch == '1' || ch == '2' || ch == '3')
        {
            pendingInterrupt_ = true;
            return true;
        }
    }
    return false;
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
        logger_.Log(Logger::LogLevel::State, "Initializing Hypervisor.");
        logger_.LogStackTrace();
        break;
    case State::Ready:
        stateMessage = "[State]: Ready\n";
        logger_.Log(Logger::LogLevel::State, "Hypervisor is ready.");
        logger_.LogStackTrace();
        break;
    case State::Running:
        stateMessage = "[State]: Running\n";
        logger_.Log(Logger::LogLevel::State, "Hypervisor is running.");
        logger_.LogStackTrace();
        break;
    case State::Stopped:
        stateMessage = "[State]: Stopped\n";
        logger_.Log(Logger::LogLevel::State, "Hypervisor is stopped.");
        logger_.LogStackTrace();
        break;
    case State::Error:
        stateMessage = "[State]: Error occurred\n";
        logger_.Log(Logger::LogLevel::Error, "An error occurred in the Hypervisor.");
        logger_.LogStackTrace();
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

HypervisorStateMachine::MenuOption HypervisorStateMachine::ShowMenu()
{
    int choice = 0;
    std::cout << "Hypervisor Menu:\n";
    std::cout << "1. Continue\n";
    std::cout << "2. Restart\n";
    std::cout << "3. Stop\n";
    std::cout << "Select an option (1-3): ";

    while (true)
    {
        if (_kbhit())
        {
            char ch = _getch();
            switch (ch)
            {
            case '1': return MenuOption::Continue;
            case '2': return MenuOption::Restart;
            case '3': return MenuOption::Stop;
            default:
                std::cout << "Invalid choice. Please select a valid option (1-3): ";
            }
        }
    }
}

bool HypervisorStateMachine::ParseArguments(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--memory") == 0 || strcmp(argv[i], "-m") == 0)
        {
            if (i + 1 < argc)
            {
                memorySize_ = std::stoull(argv[++i]);
                logger_.Log(Logger::LogLevel::Info, "Memory size set to " + std::to_string(memorySize_) + " bytes.");
            }
            else
            {
                logger_.Log(Logger::LogLevel::Error, "--memory option requires a size argument.");
                return false;
            }
        }
        else if (strcmp(argv[i], "--gui") == 0)
        {
            guiMode_ = true;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            DisplayUsage();
            return false;
        }
        else
        {
            logger_.Log(Logger::LogLevel::Error, "Unknown argument " + std::string(argv[i]));
            DisplayUsage();
            return false;
        }
    }
    return true;
}

bool HypervisorStateMachine::IsGuiMode() const
{
	return guiMode_;
}

size_t HypervisorStateMachine::GetMemorySize() const
{
	return memorySize_;
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
        logger_.Log(Logger::LogLevel::Error, "HWND is NULL in CreateDeviceD3D.");
        logger_.LogStackTrace();
        return false;
    }

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (FAILED(res))
    {
        logger_.Log(Logger::LogLevel::Error, "D3D11CreateDeviceAndSwapChain failed with HRESULT: 0x" + std::to_string(res));
        logger_.LogStackTrace();
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
    if (pBackBuffer == nullptr)
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to get back buffer from swap chain.");
        logger_.LogStackTrace();
		return;
	}
    else
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    }

    pBackBuffer->Release();
}

void HypervisorStateMachine::CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}
