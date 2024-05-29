#include <format>
#include "DeviceManager.h"
#include "Heap.h"
#include "MathUtils.h"
#include "Shader.h"
#include "CommandController.h"
#include "D3ResourceLeakChecker.h"
#include "Sphere.h"
#include "Sprite.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if(ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)){
        return true;
    }
    switch (msg){
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    D3ResourceLeakChecker leakChecker;
    //INITIALIZE COMPONENT OBJECT MODEL
    CoInitializeEx(0, COINIT_MULTITHREADED);

    //Registering Window Class
    WNDCLASS wc {};

    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = L"WindowClass";
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);


    RECT wrc = {0,0,CLIENT_WIDTH,CLIENT_HEIGHT};

    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    //Create Window 
    HWND hwnd_ = CreateWindow(
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

    ShowWindow(hwnd_, SW_SHOW);

    //=================================


    #ifdef _DEBUG
    ID3D12Debug1* debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))){
        debugController->EnableDebugLayer();

        debugController->SetEnableGPUBasedValidation(TRUE);
    }
    #endif


    //Initialize DirectX and Registering GPU

    HRESULT hResult;

    //Create Device
    Singleton<DeviceManager>::getInstance()->RegisteringDevice();

    #define device Singleton<DeviceManager>::getInstance()->getDevice().Get()
    #define dxgiFactory Singleton<DeviceManager>::getInstance()->getDXGIFactory().Get()

    //================================================================================



    #ifdef _DEBUG
    ID3D12InfoQueue* infoQueue = nullptr;
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

        infoQueue->Release();
    }

    #endif

    #define cmd Singleton<CommandController>::getInstance()

    // Create Command //
    cmd->Generate();


    //Command Queue
    ID3D12CommandQueue* commandQueue = cmd->getCommandQueue().Get();


    //Command Alloc
    ID3D12CommandAllocator* commandAllocator = cmd->getAlloc().Get();


    //Command List
    ID3D12GraphicsCommandList* commandList = cmd->getList().Get();

    //Swap Chain
    IDXGISwapChain4* swapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc {};
    swapChainDesc.Width = CLIENT_WIDTH;
    swapChainDesc.Height = CLIENT_HEIGHT;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hResult = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd_, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
    assert(SUCCEEDED(hResult));

    ///Descriptor

    //rtv
    ID3D12DescriptorHeap* rtvDescriptorHeap = Heap::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

    //srv
    ID3D12DescriptorHeap* srvDescriptorHeap = Heap::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

    //SwapChainからResourceを引っ張ってくる
    ID3D12Resource* swapChainResources[2] = {nullptr};
    hResult = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
    hResult = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
    assert(SUCCEEDED(hResult));

    //rtv settings 
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
    rtvHandles[0] = rtvStartHandle;
    device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
    rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);

    ID3D12Fence* fence = nullptr;
    uint64_t fenceValue = 0;
    hResult = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    assert(SUCCEEDED(hResult));

    HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
    assert(fenceEvent != nullptr);

    //init DxcCompiler
    IDxcUtils* dxcUtils = nullptr;
    IDxcCompiler3* dxcCompiler = nullptr;
    hResult = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    assert(SUCCEEDED(hResult));
    hResult = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    assert(SUCCEEDED(hResult));

    //support include
    IDxcIncludeHandler* includeHandler = nullptr;
    hResult = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
    assert(SUCCEEDED(hResult));


    //Sampler
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = { };
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    //compile shader
    IDxcBlob* vertexShaderBlob = Shader::CompileShader(L"Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(vertexShaderBlob != nullptr);

    IDxcBlob* pixelShaderBlob = Shader::CompileShader(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(pixelShaderBlob != nullptr);

    ///create vertex resource
    ID3D12Resource* vertexResource = Shader::CreateBufferResource(device, sizeof(VertexData) * (3/*3角形*/ * 2 /*個数*/));


    VertexData* vertexData = nullptr;

    //DepthStencilTextureを作成
    ID3D12Resource* depthStencilResource = TextureManager::CreateDepthStencilTextureResource(device, CLIENT_WIDTH, CLIENT_HEIGHT);

    //DepthStencilState
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc {};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    //DepthStencilView (DSV用のDescriptorHeapの作成)
    ID3D12DescriptorHeap* dsvDescriptorHeap = Heap::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
    //DSV setting
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    ////vertex buffer view
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView {};
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(VertexData) * (3 /*角形*/ * 2 /*枚*/);
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    ///PipelineStateObject (PSO)

    //RootSignature
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature {};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ///RootParameter
    D3D12_ROOT_PARAMETER rootParameters[3] = { };

    //pixel shader
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    //vertex shader
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 0;

    //DescriptorRange
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    //Descriptor Table
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    //set Sampler
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    //serialize
    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    hResult = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hResult)){
        System::Debug::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    ID3D12RootSignature* rootSignature = nullptr;
    hResult = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hResult));

    //InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = { };
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

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

    //PSO Desc setting
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc {};
    graphicsPipelineStateDesc.pRootSignature = rootSignature;
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
    graphicsPipelineStateDesc.BlendState = blendDesc;
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ID3D12PipelineState* graphicsPipelineState = nullptr;
    hResult = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hResult));

    //set vertexResource//リソースにデータを書き込む
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    //1枚目
    //LeftBtm
    vertexData[0].position = {-0.5f, -0.5f, 0, 1};
    vertexData[0].texCoord = {0, 1};

    //Top
    vertexData[1].position = {0, 0.5f, 0, 1};
    vertexData[1].texCoord = {0.5f, 0};

    //RightBtm
    vertexData[2].position = {0.5f, -0.5f, 0, 1};
    vertexData[2].texCoord = {1, 1};

    //2枚目
    //LeftBtm
    vertexData[3].position = {-0.5f, -0.5f, 0.5f, 1.f};
    vertexData[3].texCoord = {0,1};

    //Top
    vertexData[4].position = {0, 0, 0, 1};
    vertexData[4].texCoord = {0.5f, 0};

    //RightBtm
    vertexData[5].position = {0.5f, -0.5f, -0.5f, 1};
    vertexData[5].texCoord = {1, 1};

    System::Debug::Log(System::Debug::ConvertString(std::format(L"[Debug] : VertexResource\n")));

    //set DepthStencilState
    device->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    ///Viewport Scissor

    //Viewport
    D3D12_VIEWPORT viewport {};
    viewport.Width = CLIENT_WIDTH;
    viewport.Height = CLIENT_HEIGHT;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;

    //scissor
    D3D12_RECT scissorRect {};
    scissorRect.left = 0;
    scissorRect.right = CLIENT_WIDTH;
    scissorRect.top = 0;
    scissorRect.bottom = CLIENT_HEIGHT;


    ///EO PSO

    //Vertex MaterialResource
    ID3D12Resource* materialResource = Shader::CreateBufferResource(device, sizeof(VertexData) * (3/*角形*/ * 2/*枚*/));
    Vector4* materialData = nullptr;
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    //*materialData = {0.5f, 0.5f, 0, 1};
    *materialData = {1, 1, 1, 1};

    //Transform
    Transform transform {{1.f,1.f,1.f}, {0.f,0.f,0.f}, {0.f,0.f,0.f}},
        cameraTransform {{1,1,1}, {0,0,0}, {0,0,-5}};
    Matrix4x4 worldMatrix {}, cameraMatrix {}, viewMatrix {}, projectionMatrix {}, worldViewProjectionMatrix {};

    //WVP
    ID3D12Resource* wvpResource = Shader::CreateBufferResource(device, sizeof(Matrix4x4));
    Matrix4x4* wvpData = nullptr;
    wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
    *wvpData = MathUtils::Matrix::MakeIdentity();

    //LoadTexture
    /*DirectX::ScratchImage mipImages = Singleton<TextureManager>::getInstance()->LoadTexture("resources/uvChecker.png");
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
    ID3D12Resource* textureResource = TextureManager::CreateTextureResource(device, metadata);
    Singleton<TextureManager>::getInstance()->UploadTexture(textureResource, mipImages);
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = Singleton<TextureManager>::getInstance()->MakeSRV(metadata, srvDescriptorHeap, device, textureResource);*/

    std::shared_ptr<Texture> texture(new Texture("Resources/uvChecker.png", srvDescriptorHeap));


    ///Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX12_Init(device, swapChainDesc.BufferCount, rtvDesc.Format, srvDescriptorHeap, srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    //ImGUI用の色ステータス
    //float color[4] = {materialData->x, materialData->y, materialData->z, materialData->w};

    Sprite* sprite = new Sprite;
    sprite->Initialize(texture->getHandle());

    /*Sphere* sphere = new Sphere;
    sphere->Initialize(textureSrvHandleGPU);*/

    ///MainLoop
    MSG msg {};
    while (msg.message != WM_QUIT){
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else{
            ///FrameBegin
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            ///do somethings.../// Update
            ImGui::ShowDemoWindow();


            /*
             * ImGui
             */
#ifdef _DEBUG
            ImGui::Begin("Debug");
            ImGui::SliderFloat4("Color", &materialData->x, 0, 1);
            //*materialData = color;
            ImGui::End();
#endif

            transform.rotate.y += 0.01f;
             worldMatrix = MathUtils::Matrix::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
            cameraMatrix = MathUtils::Matrix::MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
            viewMatrix = cameraMatrix.Inverse();
            projectionMatrix = MathUtils::Matrix::MakePerspectiveFovMatrix(0.45f, static_cast<float>(CLIENT_HEIGHT) / static_cast<float>(CLIENT_WIDTH), 0.1f, 100.f);
            worldViewProjectionMatrix = worldMatrix * (viewMatrix * projectionMatrix);
            *wvpData = worldViewProjectionMatrix;
            *wvpData = worldMatrix;

            /*
             * Sprite Update
             */
            sprite->Update();

            //sphere->Update();

            ///back/// Render
            ImGui::Render();

            UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

            D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

            D3D12_RESOURCE_BARRIER barrier {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = swapChainResources[backBufferIndex];
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

            commandList->ResourceBarrier(1, &barrier);

            //commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
            commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);

            commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

            //back pic
            float clearColor[] = {0.25f, 0.1f, 0.5f, 1.0f};
            commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

            //descriptor heap for render(imgui)
            ID3D12DescriptorHeap* descriptorHeaps[] = {srvDescriptorHeap};
            commandList->SetDescriptorHeaps(1, descriptorHeaps);
            
            //stack command

            //general
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            commandList->SetGraphicsRootSignature(rootSignature);
            commandList->SetPipelineState(graphicsPipelineState);

            ////DrawTriangle
            //描画するものが三角形と登録
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            //commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

            ////setting cbv
            //commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
            //commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

            ////setting srv descriptor table
            //commandList->SetGraphicsRootDescriptorTable(2, texture->getHandle());
            ////render
            //commandList->DrawInstanced(3/*角形*/ * 2/*枚*/, 1, 0, 0);

            //sphere->Draw();

            //2D描画
            sprite->Draw();

            //IMGUI RENDER
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

            //
            //Command 詰み終わり
            //

            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

            commandList->ResourceBarrier(1, &barrier);

            hResult = commandList->Close();

            assert(SUCCEEDED(hResult));

            ID3D12CommandList* commandLists[] = {commandList};
            commandQueue->ExecuteCommandLists(1, commandLists);
            swapChain->Present(1, 0);

            ++fenceValue;
            commandQueue->Signal(fence, fenceValue);

            if (fence->GetCompletedValue() < fenceValue){
                fence->SetEventOnCompletion(fenceValue, fenceEvent);
                WaitForSingleObject(fenceEvent, INFINITE);
            }

            hResult = commandAllocator->Reset();
            assert(SUCCEEDED(hResult));
            hResult = commandList->Reset(commandAllocator, nullptr);
            assert(SUCCEEDED(hResult));

        }
    }

    //end imgui
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    sprite->Release();
    delete sprite;
    /*sphere->Release();
    delete sphere;*/

    dsvDescriptorHeap->Release();
    depthStencilResource->Release();
    wvpResource->Release();
    materialResource->Release();
    vertexResource->Release();
    graphicsPipelineState->Release();
    signatureBlob->Release();
    if (errorBlob){
        errorBlob->Release();
    }
    rootSignature->Release();
    pixelShaderBlob->Release();
    vertexShaderBlob->Release();

    CloseHandle(fenceEvent);
    fence->Release();
    srvDescriptorHeap->Release();
    rtvDescriptorHeap->Release();
    swapChainResources[0]->Release();
    swapChainResources[1]->Release();
    swapChain->Release();
    #ifdef _DEBUG
    debugController->Release();
    #endif
    CloseWindow(hwnd_);
    //QUIT COMPONENT OBJECT MODEL
    CoUninitialize();

    return 0;
}