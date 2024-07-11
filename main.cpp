#include <Windows.h>
#include <cstdint>
#include <format>
#include <string>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <wrl/client.h>
#include <dxgidebug.h>
#include <dxcapi.h>

#include "Vector4.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

struct VertexData{
    Vector4 position;
};

struct Material{
    Vector4 color;
};

struct D3DLeakChecker{
	~D3DLeakChecker() {
        Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))){
            debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
            debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
        }
	}
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg){
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

std::wstring ConvertString(const std::string& str) {
    if (str.empty()){
        return std::wstring();
    }

    auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
    if (sizeNeeded == 0){
        return std::wstring();
    }
    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
    return result;
}

std::string ConvertString(const std::wstring& str) {
    if (str.empty()){
        return std::string();
    }

    auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
    if (sizeNeeded == 0){
        return std::string();
    }
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
    return result;
}

void Log(const std::string& message) {
    OutputDebugStringA(message.c_str());
}

void Log(const std::wstring& message) {
    OutputDebugStringA(ConvertString(message).c_str());
}

IDxcBlob* CompileShader(
    const std::wstring& filePath,
    const wchar_t* profile,
    IDxcUtils* utils,
    IDxcCompiler3* compiler,
    IDxcIncludeHandler* includeHandler
) {
    Log(ConvertString(std::format(L"Begin Compile Shader , Path : {}, Profile : {}", filePath, profile)));

    IDxcBlobEncoding* shaderSource = nullptr;
    HRESULT hr = utils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    assert(SUCCEEDED(hr));

    DxcBuffer shaderSourceBuffer;
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8;

    LPCWSTR arguments[] = {
        L"-E", L"main",
        L"-T", profile,
        L"-Zi", L"-Qembed_debug",
        L"-Od", L"-Zpr"
    };

    IDxcResult* shaderResult = nullptr;
    hr = compiler->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler,
        IID_PPV_ARGS(&shaderResult)
    );
    assert(SUCCEEDED(hr));

    IDxcBlobUtf8* shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError != nullptr && shaderError->GetStringLength() != 0){
        Log(shaderError->GetStringPointer());
        assert(false);
    }

    IDxcBlob* shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));

    Log(ConvertString(std::format(L"Compile Succeeded, Path : {}, Profile : {}", filePath, profile)));

    shaderSource->Release();
    shaderResult->Release();

    return shaderBlob;
}

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
    D3D12_HEAP_PROPERTIES uploadHeapProperties {};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    //resource setting
    D3D12_RESOURCE_DESC materialResourceDesc {};
    materialResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    materialResourceDesc.Width = sizeInBytes;
    materialResourceDesc.Height = 1;
    materialResourceDesc.DepthOrArraySize = 1;
    materialResourceDesc.MipLevels = 1;
    materialResourceDesc.SampleDesc.Count = 1;
    materialResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* materialResource = nullptr;

    #ifdef _DEBUG
    HRESULT hR =
        #endif

        device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &materialResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource));

    assert(SUCCEEDED(hR));

    return materialResource;
}

//Rect
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    D3DLeakChecker leakChecker;

    //Registering Window Class
    WNDCLASS wc {};

    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = L"WindowClass";
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);


    RECT wrc = {0,0,kClientWidth,kClientHeight};

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
    if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))){
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(true);
    }
	#endif


    ShowWindow(hwnd, SW_SHOW);

    //=================================

    //Initialize DirectX and Registering GPU

    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

    assert(SUCCEEDED(hr));

    Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
    for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i){
        DXGI_ADAPTER_DESC3 adapterDesc {};
        hr = useAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));

        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)){
            Log(std::format(L"Use Adapter:{}\n", adapterDesc.Description));
            break;
        }
        useAdapter = nullptr;
    }

    assert(useAdapter != nullptr);

    Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
    };
    const char* featureLevelStrings[] = {"12.2", "12.1", "12.0"};

    for (size_t i = 0; i < _countof(featureLevels); ++i){
        hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));

        if (SUCCEEDED(hr)){
            Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
            break;
        }
    }

    assert(device != nullptr);
    Log(ConvertString(std::format(L"Complete create D3D12Device!!!\n")));

	#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))){
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

        D3D12_MESSAGE_ID denyIds[] = {
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
        };

        D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
        D3D12_INFO_QUEUE_FILTER filter {};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;

        infoQueue->PushStorageFilter(&filter);
    }
	#endif


    //================================================================================

    //Create CommandQueue
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc {};
    hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));

    assert(SUCCEEDED(hr));

    //Command Alloc
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    assert(SUCCEEDED(hr));

    //Command List
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
    assert(SUCCEEDED(hr));

    //SwapChain
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc {};
    swapChainDesc.Width = kClientWidth;
    swapChainDesc.Height = kClientHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
    assert(SUCCEEDED(hr));

    //rtv

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc {};
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescriptorHeapDesc.NumDescriptors = 2;

    hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
    assert(SUCCEEDED(hr));

    Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = {nullptr};

    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
    assert(SUCCEEDED(hr));

    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
    assert(SUCCEEDED(hr));

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
    rtvHandles[0] = rtvStartHandle;
    device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);

    rtvHandles[1].ptr = rtvStartHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);

    //fence
    Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
    uint64_t fenceValue = 0;
    hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    assert(SUCCEEDED(hr));

    HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
    assert(fenceEvent != nullptr);

    //dxc
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;

    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    assert(SUCCEEDED(hr));

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    assert(SUCCEEDED(hr));

    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
    hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);

#pragma region PSO

    /*
     * RootSignature
     * InputLayout
     * BlendState
     * VertexShader
     * Rasterizer
     * PixelShader
     */

    //RootSignature
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature {};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    //RootParameter
    D3D12_ROOT_PARAMETER rootParameter[1] = {};
    rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameter[0].Descriptor.ShaderRegister = 0;

    descriptionRootSignature.pParameters = rootParameter;
    descriptionRootSignature.NumParameters = _countof(rootParameter);

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

    if(FAILED(hr)){
        Log(static_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));

    //InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc {};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    //BlendState
    D3D12_BLEND_DESC blendDesc {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    //RasterizerState
    D3D12_RASTERIZER_DESC rasterizerDesc {};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    //Compile Shader
    Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"Object3d.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
    assert(vertexShaderBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
    assert(pixelShaderBlob != nullptr);

    //pipeline state object
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc {};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
    graphicsPipelineStateDesc.BlendState = blendDesc;
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};

    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
    hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));
#pragma endregion

    //Viewport Scissor
    D3D12_VIEWPORT viewport {};
    viewport.Width = kClientWidth;
    viewport.Height = kClientHeight;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;

    D3D12_RECT scissorRect {};
    scissorRect.left = 0;
    scissorRect.right = kClientWidth;
    scissorRect.top = 0;
    scissorRect.bottom = kClientHeight;

    //Triangle
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
    vertexResource.Attach(CreateBufferResource(device.Get(), sizeof(VertexData) * 3));
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
    materialResource.Attach(CreateBufferResource(device.Get(), sizeof(Material)));

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView {};
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(VertexData) * 3;
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    vertexData[0].position = {-0.5f, -0.5f, 0.f, 1.f};
    vertexData[1].position = {0.f, 0.5f, 0.f, 1.f};
    vertexData[2].position = {0.5f, -0.5f, 0.f, 1.f};

    Material* materialData = nullptr;
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

    materialData->color = {1, 0, 0, 1};

    MSG msg {};
    while(msg.message != WM_QUIT){
        if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }else{
            //do somethings...//

            UINT bbi = swapChain->GetCurrentBackBufferIndex();

            D3D12_RESOURCE_BARRIER barrier {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = swapChainResources[bbi].Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

            commandList->ResourceBarrier(1, &barrier);

            commandList->OMSetRenderTargets(1, &rtvHandles[bbi], false, nullptr);

            float color[4] = {0.1f, 0.25f, 0.5f, 1};
            commandList->ClearRenderTargetView(rtvHandles[bbi], color, 0, nullptr);

            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            commandList->SetGraphicsRootSignature(rootSignature.Get());

            commandList->SetPipelineState(graphicsPipelineState.Get());

#pragma region Draw
            //Triangle
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
            commandList->DrawInstanced(3, 1, 0, 0 );


#pragma endregion


            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

            commandList->ResourceBarrier(1, &barrier);

            hr = commandList->Close();
            assert(SUCCEEDED(hr));

            Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[] = {commandList.Get()};
            commandQueue->ExecuteCommandLists(1, commandLists->GetAddressOf());

            swapChain->Present(1, 0);

            ++fenceValue;
            commandQueue->Signal(fence.Get(), fenceValue);

            if (fence->GetCompletedValue() < fenceValue){
                fence->SetEventOnCompletion(fenceValue, fenceEvent);
                WaitForSingleObject(fenceEvent, INFINITE);
            }

            hr = commandAllocator.Get()->Reset();
            assert(SUCCEEDED(hr));

            hr = commandList.Get()->Reset(commandAllocator.Get(), nullptr);
            assert(SUCCEEDED(hr));
        }
    }

    CloseHandle(fenceEvent);
    CloseWindow(hwnd);


    return 0;
}

