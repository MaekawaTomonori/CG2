#include "Application.h"

#include <cstdint>
#include <d3d12sdklayers.h>
#include <windows.h>
#include <wrl/client.h>

#include "imgui/imgui.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)){
        return true;
    }
    switch (msg){
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Application::Initialize(int32_t width, int32_t height) {
    //Registering Window Class
    WNDCLASS wc {};

    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = L"WindowClass";
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);


    RECT wrc = {0,0,width,height};

    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    //Create Window 
    HWND hwnd = CreateWindow(
        wc.lpszClassName,
        L"CG2",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    #ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))){
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(true);
    }
    #endif


    ShowWindow(hwnd, SW_SHOW);

    UpdateWindow(hwnd);
}

void Application::Update() {
}
