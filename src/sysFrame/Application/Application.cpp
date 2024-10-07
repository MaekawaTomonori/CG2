#include "Application.h"

#include <cstdint>
#include <d3d12sdklayers.h>
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

bool Application::Create() {
	//Registering Window Class
    wc_.lpfnWndProc = WindowProc;
    wc_.lpszClassName = L"WindowClass";
    wc_.hInstance = GetModuleHandle(nullptr);
    wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc_);


    RECT wrc = {0,0,kClientWidth,kClientHeight};

    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    //Create Window 
    hwnd_ = CreateWindow(
        wc_.lpszClassName,
        L"CG2",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        wc_.hInstance,
        nullptr
    );

    #ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))){
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(true);
    }
    #endif


    ShowWindow(hwnd_, SW_SHOW);

    UpdateWindow(hwnd_);
    return true;
}

void Application::Initialize() {
	CoInitializeEx(0, COINIT_MULTITHREADED);
    Create();
}

void Application::Update() {
}

void Application::Finalize() const {
    CloseWindow(hwnd_);
    CoUninitialize();
}

bool Application::ProcessMessage() {
    MSG msg;
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)){
	    if (msg.message == WM_QUIT){
		    return false;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    return true;
}

HWND Application::GetHwnd() const {
    return hwnd_;
}

WNDCLASS Application::GetWindowClass() const {
    return wc_;
}
