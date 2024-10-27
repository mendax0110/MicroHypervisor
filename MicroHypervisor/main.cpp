#include "HypervisorStateMachine.h"
#include <iostream>
#include <sstream>
#include <string>
#include <Windows.h>
#include <tchar.h>

HypervisorStateMachine* globalHypervisor = nullptr;
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        if (globalHypervisor && globalHypervisor->g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            globalHypervisor->CleanupRenderTarget();
            globalHypervisor->g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            globalHypervisor->CreateRenderTarget();
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

int main(int argc, char* argv[])
{
    size_t memorySize = 0x400000; // Default memory size: 4MB

    HypervisorStateMachine tempHypervisor(memorySize);

    if (!tempHypervisor.ParseArguments(argc, argv))
    {
        return EXIT_FAILURE; 
    }

    HypervisorStateMachine hypervisor(tempHypervisor.GetMemorySize());
    globalHypervisor = &hypervisor;

    if (hypervisor.IsGuiMode())
    {
        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("MicroHypervisor"), NULL };
        RegisterClassEx(&wc);

        HWND hwnd = CreateWindow(wc.lpszClassName, _T("MicroHypervisor GUI"), WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);
        if (!hwnd)
        {
            std::cerr << "[Error]: Window creation failed." << std::endl;
            return EXIT_FAILURE;
        }

        if (!hypervisor.CreateDeviceD3D(hwnd))
        {
            std::cerr << "[Error]: Failed to create Direct3D device." << std::endl;
            hypervisor.CleanupDeviceD3D();
            UnregisterClass(wc.lpszClassName, wc.hInstance);
            return EXIT_FAILURE;
        }

        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);
        hypervisor.RunGUI(hwnd);

        // Cleanup
        hypervisor.CleanupDeviceD3D();
        DestroyWindow(hwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 0;
    }

    hypervisor.Start();

    return EXIT_SUCCESS;
}
