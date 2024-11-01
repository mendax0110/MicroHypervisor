#define NOMINMAX
#include "HypervisorStateMachine.h"
#include <iostream>
#include <limits>
#include <conio.h>
#include "Registers.h"

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
    memoryManager_.UpdateMemorySize(memorySize_);
    RunHypervisor();
}

void HypervisorStateMachine::Stop()
{
    running_ = false;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HypervisorStateMachine* hypervisor = reinterpret_cast<HypervisorStateMachine*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (hypervisor && hypervisor->g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            hypervisor->CleanupRenderTarget();
            hypervisor->g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            hypervisor->CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_CLOSE)
        {
            PostQuitMessage(0);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void HypervisorStateMachine::RunGui()
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("MicroHypervisor"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("MicroHypervisor GUI"), WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);

    if (!hwnd)
    {
        std::cerr << "[Error]: Failed to create GUI window.\n";
        logger_.Log(Logger::LogLevel::Error, "Failed to create GUI window.");
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return;
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    if (!CreateDeviceD3D(hwnd))
    {
        std::cerr << "[Error]: Failed to create Direct3D device.\n";
        logger_.Log(Logger::LogLevel::Error, "Failed to create Direct3D device.");
        CleanupDeviceD3D();
        DestroyWindow(hwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.ItemSpacing = ImVec2(10, 5);
    style.WindowPadding = ImVec2(10, 10);

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    static size_t newMemorySize = memorySize_;

    logger_.SetLogCallback([this](const std::string& logMessage) {
        outputBuffer << logMessage << "\n";
    });

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

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit"))
                {
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Hypervisor"))
            {
                if (ImGui::MenuItem("Start"))
                {
                    HandleMenuOption(MenuOption::Start);
                }
                if (ImGui::MenuItem("Stop"))
                {
                    HandleMenuOption(MenuOption::Stop);
                }
                if (ImGui::MenuItem("Restart"))
                {
                    HandleMenuOption(MenuOption::Restart);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save Snapshot"))
                {
                    HandleMenuOption(MenuOption::SaveSnapshot);
                }
                if (ImGui::MenuItem("Restore Snapshot"))
                {
                    HandleMenuOption(MenuOption::RestoreSnapshot);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Dump Registers"))
                {
                    HandleMenuOption(MenuOption::DumpRegisters);
                }
                if (ImGui::MenuItem("Set Registers"))
                {
                    HandleMenuOption(MenuOption::SetRegisters);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Advanced"))
            {
                if (ImGui::MenuItem("Get Registers"))
                {
                    HandleMenuOption(MenuOption::GetRegisters);
                }
                if (ImGui::MenuItem("Set Specific Register"))
                {
                    HandleMenuOption(MenuOption::SetSpecificRegister);
                }
                if (ImGui::MenuItem("Get Specific Register"))
                {
                    HandleMenuOption(MenuOption::GetSpecificRegister);
                }
                if (ImGui::MenuItem("Configure VM"))
                {
                    HandleMenuOption(MenuOption::ConfigureVM);
                }
                if (ImGui::MenuItem("Get VM Config"))
                {
                    HandleMenuOption(MenuOption::GetVMConfig);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Settings"))
            {
                ImGui::InputScalar("Set Memory Size", ImGuiDataType_U64, &newMemorySize, NULL, NULL, "%zu", ImGuiInputTextFlags_CharsHexadecimal);
                if (ImGui::Button("Update Memory Size"))
                {
                    if (newMemorySize > 0)
                    {
                        memorySize_ = newMemorySize;
                        memoryManager_.UpdateMemorySize(memorySize_);
                        logger_.Log(Logger::LogLevel::Info, "Memory size updated to " + std::to_string(memorySize_) + " bytes.");
                    }
                    else
                    {
                        logger_.Log(Logger::LogLevel::Error, "Invalid memory size: " + std::to_string(newMemorySize));
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        ImGui::Begin("Resource Monitoring");
        ImGui::Text("CPU Usage: %.1f%%", virtualProcessor_->GetCPUUsage());
        //ImGui::Text("Memory Usage: %zu / %zu bytes", memoryManager_.GetCurrentUsage(), memorySize_);
        ImGui::Text("Active Threads: %d", virtualProcessor_->GetActiveThreadCount());

        const char* stateStr = "Unknown";
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            switch (currentState_)
            {
            case State::Initializing: stateStr = "Initializing"; break;
            case State::Ready: stateStr = "Ready"; break;
            case State::Running: stateStr = "Running"; break;
            case State::Stopped: stateStr = "Stopped"; break;
            case State::Error: stateStr = "Error"; break;
            }
        }

        ImGui::Text("Current State: %s", stateStr);

        {
            std::lock_guard<std::mutex> lock(outputMutex);
            ImGui::BeginChild("Output", ImVec2(0, 200), true);
            ImGui::TextWrapped("%s", outputBuffer.str().c_str());
            ImGui::EndChild();
        }

        ImGui::End();
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
}

void HypervisorStateMachine::RunCli()
{
    std::string input;

    while (true)
    {
        DisplayUsageAndMenu(); // Call the merged function

        while (!_kbhit())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        char ch = _getch();
        if (ch == 'q' || ch == 'Q')
        {
            break;
        }

        switch (ch) {
        case '1':
            HandleMenuOption(MenuOption::Start);
            break;
        case '2':
            HandleMenuOption(MenuOption::Stop);
            break;
        case '3':
            HandleMenuOption(MenuOption::Restart);
            break;
        case '4':
            HandleMenuOption(MenuOption::SaveSnapshot);
            break;
        case '5':
            HandleMenuOption(MenuOption::RestoreSnapshot);
            break;
        case '6':
            HandleMenuOption(MenuOption::DumpRegisters);
            break;
        case '7':
            HandleMenuOption(MenuOption::SetRegisters);
            break;
        case '8':
            HandleMenuOption(MenuOption::GetRegisters);
            break;
        case '9':
            UpdateMemorySize();
            break;
        default:
            std::cout << "Unknown command. Please try again.\n";
            break;
        }
    }
}

void HypervisorStateMachine::UpdateMemorySize()
{
    std::cout << "Enter new memory size (in bytes): ";
    std::size_t newSize;
    std::cin >> newSize;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (newSize > 0)
    {
        memorySize_ = newSize;
        std::cout << "Memory size updated to " << memorySize_ << " bytes.\n";
    }
    else
    {
        std::cout << "Invalid memory size.\n";
    }
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

void HypervisorStateMachine::HandleMenuOption(MenuOption option)
{
    switch (option)
    {
    case MenuOption::Continue:
        if (!running_)
        {
            running_ = true;
            logger_.Log(Logger::LogLevel::Error, "Hypervisor is not running.");
            TransitionState(State::Running);
            std::thread([this]() { RunHypervisor(); }).detach();
        }
        else
        {
            logger_.Log(Logger::LogLevel::Info, "Continue action triggered.");
            logger_.LogStackTrace();
        }
        break;
    case MenuOption::Restart:
        if (virtualProcessor_ != nullptr)
        {
            virtualProcessor_->RestoreState();
            running_ = true;
            logger_.Log(Logger::LogLevel::Info, "Restart action triggered.");
            TransitionState(State::Running);
            std::thread([this]() { RunHypervisor(); }).detach();
        }
        else
        {
            logger_.Log(Logger::LogLevel::Error, "VirtualProcessor instance is null.");
			logger_.LogStackTrace();
        }
		break;
    case MenuOption::Stop:
		running_ = false;
        if (virtualProcessor_ != nullptr)
		{
			virtualProcessor_->SaveState();
		}
        logger_.Log(Logger::LogLevel::Info, "Stop action triggered.");
        TransitionState(State::Stopped);
		break;
    case MenuOption::Start:
		if (!running_)
		{
			running_ = true;
			logger_.Log(Logger::LogLevel::Info, "Start action triggered.");
			TransitionState(State::Running);
            std::thread([this]() { Start(); }).detach();
		}
        else
        {
            logger_.Log(Logger::LogLevel::Error, "Hypervisor is already running.");
            logger_.LogStackTrace();
        }
        break;
    case MenuOption::SaveSnapshot:
        snapshotManager_.SaveSnapshot();
        break;
    case MenuOption::RestoreSnapshot:
        snapshotManager_.RestoreSnapshot();
		break;
    case MenuOption::DumpRegisters:
		if (virtualProcessor_ != nullptr)
		{
			virtualProcessor_->DumpRegisters();
		}
		else
		{
			logger_.Log(Logger::LogLevel::Error, "VirtualProcessor instance is null.");
            logger_.LogStackTrace();
        }
        break;
    case MenuOption::DetailedDumpRegisters:
        if (virtualProcessor_ != nullptr)
        {
			virtualProcessor_->DetailedDumpRegisters();
		}
		else
		{
			logger_.Log(Logger::LogLevel::Error, "VirtualProcessor instance is null.");
			logger_.LogStackTrace();
		}
        break;
    case MenuOption::SetRegisters:
		if (virtualProcessor_ != nullptr)
		{
			virtualProcessor_->SetRegisters();
		}
		else
		{
			logger_.Log(Logger::LogLevel::Error, "VirtualProcessor instance is null.");
			logger_.LogStackTrace();
		}
		break;
    case MenuOption::GetRegisters:
        if (virtualProcessor_ != nullptr)
        {
			virtualProcessor_->GetRegisters();
		}
		else
		{
			logger_.Log(Logger::LogLevel::Error, "VirtualProcessor instance is null.");
			logger_.LogStackTrace();
		}
        break;
    case MenuOption::SetSpecificRegister:
        if (virtualProcessor_ != nullptr)
		{
			virtualProcessor_->SetSpecificRegister(WHvX64RegisterRip, 0x1000);
			virtualProcessor_->SetSpecificRegister(WHvX64RegisterRflags, 0x2);
			virtualProcessor_->SetSpecificRegister(WHvX64RegisterCs, 0x8);
		}
		else
		{
			logger_.Log(Logger::LogLevel::Error, "VirtualProcessor instance is null.");
			logger_.LogStackTrace();
		}
		break;
    case MenuOption::GetSpecificRegister:
        if (virtualProcessor_ != nullptr)
        {
            virtualProcessor_->GetSpecificRegister(WHvX64RegisterRip);
            virtualProcessor_->GetSpecificRegister(WHvX64RegisterRflags);
            virtualProcessor_->GetSpecificRegister(WHvX64RegisterCs);
        }
        else
        {
            logger_.Log(Logger::LogLevel::Error, "VirtualProcessor instance is null.");
            logger_.LogStackTrace();
        }
        break;
    case MenuOption::ConfigureVM:
        if (virtualProcessor_ != nullptr)
		{
			VirtualProcessor::VMConfig config;
			config.cpuCount = 1;
			config.memorySize = 4194304;
			config.ioDevices = "COM1, COM2, LPT1";
			virtualProcessor_->ConfigureVM(config);
		}
		else
		{
			logger_.Log(Logger::LogLevel::Error, "VirtualProcessor instance is null.");
			logger_.LogStackTrace();
		}
		break;
    case MenuOption::GetVMConfig:
        if (virtualProcessor_ != nullptr)
		{
			VirtualProcessor::VMConfig config = virtualProcessor_->GetVMConfig();
			logger_.Log(Logger::LogLevel::Info, "VM configured with " + std::to_string(config.cpuCount) + " CPU(s), " +
				std::to_string(config.memorySize) + " bytes of memory, I/O devices: " + config.ioDevices);
		}
		else
		{
			logger_.Log(Logger::LogLevel::Error, "VirtualProcessor instance is null.");
			logger_.LogStackTrace();
		}
		break;
    default:
        logger_.Log(Logger::LogLevel::Error, "Invalid menu option.");
		logger_.LogStackTrace();
        break;
    }
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
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << output;
    }
}

void HypervisorStateMachine::TransitionState(State newState)
{
    std::string stateMessage;
    std::lock_guard<std::mutex> lock(stateMutex);
    currentState_ = newState;
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

void HypervisorStateMachine::DisplayUsageAndMenu()
{
    std::cout << "#######################################################################\n";
    std::cout << "Usage: MicroHypervisor [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -m, --memory <size>   Set the memory size in bytes (default: 4194304)\n";
    std::cout << "  --gui                 Launch GUI mode\n";
    std::cout << "  -h, --help            Show this help message\n\n";

    std::cout << "Hypervisor CLI Menu:\n"
        << "1. Start\n"
        << "2. Stop\n"
        << "3. Restart\n"
        << "4. Save Snapshot\n"
        << "5. Restore Snapshot\n"
        << "6. Dump Registers\n"
        << "7. Set Registers\n"
        << "8. Get Registers\n"
        << "9. Set Memory Size\n"
        << "q. Quit\n"
        << "Select an option: ";
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
            DisplayUsageAndMenu();
            return false;
        }
        else
        {
            logger_.Log(Logger::LogLevel::Error, "Unknown argument " + std::string(argv[i]));
            DisplayUsageAndMenu();
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
    if (!hWnd)
    {
        logger_.Log(Logger::LogLevel::Error, "HWND is NULL in CreateDeviceD3D.");
        logger_.LogStackTrace();
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd = {};
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

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, ARRAYSIZE(featureLevelArray),
        D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (FAILED(res))
    {
        logger_.Log(Logger::LogLevel::Error, "D3D11CreateDeviceAndSwapChain failed with HRESULT: 0x" + std::to_string(res));
        logger_.LogStackTrace();

        if (res == DXGI_ERROR_UNSUPPORTED)
        {
            res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
                featureLevelArray, ARRAYSIZE(featureLevelArray),
                D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
            if (FAILED(res))
            {
                logger_.Log(Logger::LogLevel::Error, "WARP D3D11CreateDeviceAndSwapChain failed with HRESULT: 0x" + std::to_string(res));
                return false;
            }
        }
    }

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
    ID3D11Texture2D* pBackBuffer = nullptr;
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
