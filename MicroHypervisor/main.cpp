#include "Partition.h"
#include "VirtualProcessor.h"
#include "Emulator.h"
#include "InterruptController.h"
#include "MemoryManager.h"
#include "SnapshotManager.h"
#include "Timer.h"

#include <Windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>

#include <d3d11.h>
#include <tchar.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;
static HWND hwnd = NULL;
static std::stringstream outputBuffer;
static std::mutex outputMutex;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void DisplayUsage()
{
    std::cout << "Usage: MicroHypervisor [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -m, --memory <size>   Set the memory size in bytes (default: 4194304)\n";
    std::cout << "  --gui                  Launch GUI mode\n";
    std::cout << "  -h, --help            Show this help message\n";
}

static void CheckHypervisorCapability()
{
    WHV_CAPABILITY capability = {};
    HRESULT result = WHvGetCapability(WHvCapabilityCodeHypervisorPresent, &capability, sizeof(capability), nullptr);
    if (result != S_OK || !capability.HypervisorPresent)
    {
        std::cerr << "Error: Hypervisor platform not available. Please ensure that your system supports virtualization.\n";
        exit(EXIT_FAILURE);
    }
}

static void RunGui(size_t memorySize)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

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
        ImGui::Text("Memory Size: %zu bytes", memorySize);

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
                try
                {
                    CheckHypervisorCapability();

                    Partition partition;
                    if (!partition.Setup())
                    {
                        std::lock_guard<std::mutex> lock(outputMutex);
                        outputBuffer << "[Error]: Failed to set up the partition. Ensure you have proper permissions and virtualization enabled.\n";
                        return;
                    }

                    if (!partition.CreateVirtualProcessor(0))
                    {
                        std::lock_guard<std::mutex> lock(outputMutex);
                        outputBuffer << "[Error]: Failed to create virtual processor. Check if the partition is correctly set up.\n";
                        return;
                    }

                    VirtualProcessor vp(partition.GetHandle(), 0);
                    vp.DumpRegisters();
                    HRESULT hr = vp.SaveState();
                    if (FAILED(hr))
                    {
                        std::lock_guard<std::mutex> lock(outputMutex);
                        outputBuffer << "[Error]: Failed to save the state of the virtual processor. HRESULT: " << hr << "\n";
                        return;
                    }

                    hr = vp.RestoreState();
                    if (FAILED(hr))
                    {
                        std::lock_guard<std::mutex> lock(outputMutex);
                        outputBuffer << "[Error]: Failed to restore the state of the virtual processor. HRESULT: " << hr << "\n";
                        return;
                    }

                    Emulator emulator;
                    if (!emulator.Initialize())
                    {
                        std::lock_guard<std::mutex> lock(outputMutex);
                        outputBuffer << "[Error]: Emulator initialization failed.\n";
                        return;
                    }

                    auto sourceMemory = VirtualAlloc(nullptr, memorySize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
                    if (!sourceMemory)
                    {
                        std::lock_guard<std::mutex> lock(outputMutex);
                        outputBuffer << "[Error]: Memory allocation failed.\n";
                        return;
                    }

                    hr = WHvMapGpaRange(partition.GetHandle(), sourceMemory, 0, memorySize,
                        WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagWrite | WHvMapGpaRangeFlagExecute);
                    if (FAILED(hr))
                    {
                        std::lock_guard<std::mutex> lock(outputMutex);
                        outputBuffer << "[Error]: Failed to map guest physical address range. HRESULT: " << hr << "\n";
                        VirtualFree(sourceMemory, 0, MEM_RELEASE);
                        return;
                    }

                    {
                        std::lock_guard<std::mutex> lock(outputMutex);
                        outputBuffer << "[SUCCESS]: Hypervisor started with memory size: " << memorySize << " bytes.\n";
                    }
                    VirtualFree(sourceMemory, 0, MEM_RELEASE);
                }
                catch (const std::exception& e)
                {
                    std::lock_guard<std::mutex> lock(outputMutex);
                    outputBuffer << "[Exception]: " << e.what() << "\n";
                }
                }).detach();

            ImGui::EndDisabled();
        }
        ImGui::End();

        // Rendering
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

int main(int argc, char* argv[])
{
    size_t memorySize = 0x400000; // Default memsize: 4MB
    bool guiMode = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-m" || arg == "--memory")
        {
            if (i + 1 < argc)
            {
                std::istringstream iss(argv[++i]);
                if (!(iss >> memorySize) || memorySize == 0)
                {
                    std::cerr << "[Error]: Invalid memory size specified. Please provide a valid non-zero value.\n";
                    return EXIT_FAILURE;
                }
            }
            else
            {
                std::cerr << "[Error]: No memory size specified after -m/--memory option.\n";
                return EXIT_FAILURE;
            }
        }
        else if (arg == "--gui")
        {
            guiMode = true;
        }
        else if (arg == "-h" || arg == "--help")
        {
            DisplayUsage();
            return EXIT_SUCCESS;
        }
        else
        {
            std::cerr << "[Error]: Unknown option " << arg << ". Use -h/--help for help.\n";
            return EXIT_FAILURE;
        }
    }

    if (guiMode)
    {
        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("MicroHypervisor"), NULL };
        RegisterClassEx(&wc);

        hwnd = CreateWindow(wc.lpszClassName, _T("MicroHypervisor GUI"), WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);

        if (!hwnd)
        {
            std::cerr << "[Error]: Window creation failed." << std::endl;
            return EXIT_FAILURE;
        }

        if (!CreateDeviceD3D(hwnd))
        {
            CleanupDeviceD3D();
            UnregisterClass(wc.lpszClassName, wc.hInstance);
            return 1;
        }

        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);
        RunGui(memorySize);
        CleanupDeviceD3D();
        DestroyWindow(hwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 0;
    }


    std::cout << "[SUCCESS]: Starting MicroHypervisor with memory size: " << memorySize << " bytes.\n";

    CheckHypervisorCapability();

    Partition partition;
    if (!partition.Setup())
    {
        std::cerr << "[Error]: Failed to set up the partition. Ensure you have proper permissions and virtualization enabled.\n";
        return EXIT_FAILURE;
    }

    if (!partition.CreateVirtualProcessor(0))
    {
        std::cerr << "[Error]: Failed to create virtual processor. Check if the partition is correctly set up.\n";
        return EXIT_FAILURE;
    }

    VirtualProcessor vp(partition.GetHandle(), 0);

    // dmp initial registers
    vp.DumpRegisters();
    vp.DetailedDumpRegisters();

    InterruptController interruptController(partition.GetHandle());
    if (!interruptController.Setup())
    {
        std::cerr << "[Error]: Failed to initialize InterruptController.\n";
        return EXIT_FAILURE;
    }

    MemoryManager memoryManager(partition.GetHandle(), memorySize);
    if (!memoryManager.Initialize())
    {
        std::cerr << "[Error]: Failed to initialize MemoryManager.\n";
        return EXIT_FAILURE;
    }

    SnapshotManager snapshotManager(partition.GetHandle());
    if (!snapshotManager.Initialize())
    {
        std::cerr << "[Error]: Failed to initialize SnapshotManager.\n";
        return EXIT_FAILURE;
    }


    //Timer timer;
    //timer.Start();

    //sav current state of the virtual processor
    HRESULT hr = vp.SaveState();
    if (FAILED(hr))
    {
        std::cerr << "[Error]: Failed to save the state of the virtual processor. HRESULT: " << hr << "\n";
        return EXIT_FAILURE;
    }

    // perform operations or simulate events...
    hr = vp.RestoreState();
    if (FAILED(hr))
    {
        std::cerr << "[Error]: Failed to restore the state of the virtual processor. HRESULT: " << hr << "\n";
        return EXIT_FAILURE;
    }

    vp.DumpRegisters();
    vp.DetailedDumpRegisters();

    Emulator emulator;
    if (!emulator.Initialize())
    {
        std::cerr << "[Error]: Emulator initialization failed. Ensure the emulator environment is correctly configured.\n";
        return EXIT_FAILURE;
    }

    auto sourceMemory = VirtualAlloc(nullptr, memorySize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!sourceMemory)
    {
        std::cerr << "[Error]: Memory allocation failed. Insufficient memory or invalid parameters.\n";
        return EXIT_FAILURE;
    }

    hr = WHvMapGpaRange(partition.GetHandle(), sourceMemory, 0, memorySize,
        WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagWrite | WHvMapGpaRangeFlagExecute);
    if (FAILED(hr))
    {
        std::cerr << "[Error]: Failed to map guest physical address range. HRESULT: " << hr << "\n";
        VirtualFree(sourceMemory, 0, MEM_RELEASE);
        return EXIT_FAILURE;
    }

    std::cout << "[SUCCESS]: Memory mapped and setup complete. Ready to start the virtual machine.\n";

    VirtualFree(sourceMemory, 0, MEM_RELEASE);
    return EXIT_SUCCESS;
}

bool CreateDeviceD3D(HWND hWnd)
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

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
            CreateRenderTarget();
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_CLOSE)
            PostQuitMessage(0);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
 
 
 
 
 
//TODO: UNCOMMENT AS SOON AS WE USE THE STATE MACHINE
//#include "HypervisorStateMachine.h"
//#include <iostream>
//#include <sstream>
//#include <string>
//#include <Windows.h>
//#include <tchar.h>
//
//LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//
//int main(int argc, char* argv[])
//{
//    size_t memorySize = 0x400000; // Default memory size: 4MB
//    bool guiMode = false;
//
//    // Parse command-line arguments
//    for (int i = 1; i < argc; ++i)
//    {
//        std::string arg = argv[i];
//        if (arg == "-m" || arg == "--memory")
//        {
//            if (i + 1 < argc)
//            {
//                std::istringstream iss(argv[++i]);
//                if (!(iss >> memorySize) || memorySize == 0)
//                {
//                    std::cerr << "[Error]: Invalid memory size specified. Please provide a valid non-zero value.\n";
//                    return EXIT_FAILURE;
//                }
//            }
//            else
//            {
//                std::cerr << "[Error]: No memory size specified after -m/--memory option.\n";
//                return EXIT_FAILURE;
//            }
//        }
//        else if (arg == "--gui")
//        {
//            guiMode = true;
//        }
//        else if (arg == "-h" || arg == "--help")
//        {
//            std::cout << "Usage: MicroHypervisor [options]\n";
//            std::cout << "Options:\n";
//            std::cout << "  -m, --memory <size>   Set the memory size in bytes (default: 4194304)\n";
//            std::cout << "  --gui                  Launch GUI mode\n";
//            std::cout << "  -h, --help            Show this help message\n";
//            return EXIT_SUCCESS;
//        }
//        else
//        {
//            std::cerr << "[Error]: Unknown option " << arg << ". Use -h/--help for help.\n";
//            return EXIT_FAILURE;
//        }
//    }
//
//    // Create the hypervisor state machine instance
//    HypervisorStateMachine hypervisor(memorySize);
//
//    if (guiMode)
//    {
//        // Setup GUI Window and Direct3D device
//        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("MicroHypervisor"), NULL };
//        RegisterClassEx(&wc);
//
//        HWND hwnd = CreateWindow(wc.lpszClassName, _T("MicroHypervisor GUI"), WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);
//        if (!hwnd)
//        {
//            std::cerr << "[Error]: Window creation failed." << std::endl;
//            return EXIT_FAILURE;
//        }
//
//        if (!hypervisor.CreateDeviceD3D(hwnd))
//        {
//            std::cerr << "[Error]: Failed to create Direct3D device." << std::endl;
//            hypervisor.CleanupDeviceD3D();
//            UnregisterClass(wc.lpszClassName, wc.hInstance);
//            return EXIT_FAILURE;
//        }
//
//        ShowWindow(hwnd, SW_SHOWDEFAULT);
//        UpdateWindow(hwnd);
//        hypervisor.RunGui();  // Run the GUI
//
//        hypervisor.CleanupDeviceD3D();  // Cleanup Direct3D resources
//        DestroyWindow(hwnd);
//        UnregisterClass(wc.lpszClassName, wc.hInstance);
//        return 0;
//    }
//
//    // Non-GUI mode initialization
//    //if (!hypervisor.RunHypervisor())
//    //{
//    //    std::cerr << "[Error]: Hypervisor initialization failed.\n";
//    //    return EXIT_FAILURE;
//    //}
//
//    hypervisor.Start(); // Start the hypervisor state machine
//
//    return EXIT_SUCCESS;
//}

