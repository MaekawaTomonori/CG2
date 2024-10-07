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
#include <numbers>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <array>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

//externals
#include "DirectXMath.h"
#include "DirectXTex/d3dx12.h"
#include "DirectXTex/DirectXTex.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"


#include "Application/Application.h"
#include "input/Input.h"

#include "math/MathUtils.h"
#include "math/Matrix.h"
#include "math/Transform.h"
#include "math/Vector2.h"
#include "math/Vector4.h"


struct VertexData{
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
};

struct Material{
    Vector4 color;
    int32_t enableLighting;
    float pad[3];
    Matrix4x4 uvTransform;
};

struct TransformationMatrix{
    Matrix4x4 WVP;
    Matrix4x4 World;
};

struct DirectionalLight{
    Vector4 color;
    Vector3 direction;
    float intensity;
};

struct MaterialData{
    std::string textureFilePath;
};

struct ModelData{
    std::vector<VertexData> vertices;
    MaterialData materialData;
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

Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
    const std::wstring& filePath,
    const wchar_t* profile,
    const Microsoft::WRL::ComPtr<IDxcUtils>& utils,
    const Microsoft::WRL::ComPtr<IDxcCompiler3>& compiler,
    const Microsoft::WRL::ComPtr<IDxcIncludeHandler>& includeHandler
) {
    Log(ConvertString(std::format(L"Begin Compile Shader , Path : {}, Profile : {}\n", filePath, profile)));

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
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

    Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
    hr = compiler->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler.Get(),
        IID_PPV_ARGS(&shaderResult)
    );
    assert(SUCCEEDED(hr));

    Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError != nullptr && shaderError->GetStringLength() != 0){
        Log(shaderError->GetStringPointer());
        assert(false);
    }

    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));

    Log(ConvertString(std::format(L"Compile Succeeded, Path : {}, Profile : {}\n", filePath, profile)));

    return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes) {
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

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;

    #ifdef _DEBUG
    HRESULT hR =
        #endif

        device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &materialResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource));

    assert(SUCCEEDED(hR));

    return materialResource;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT descriptorsNum, bool shaderVisible) {
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = type;
    heapDesc.NodeMask = 0;
    heapDesc.NumDescriptors = descriptorsNum;
    heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
    assert(SUCCEEDED(hr));

    return heap;
}


DirectX::ScratchImage LoadTexture(const std::string& filePath) {
    DirectX::ScratchImage image;
    std::wstring filePathW = ConvertString(filePath);
    HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
    assert(SUCCEEDED(hr));

    DirectX::ScratchImage mipImages {};
    hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
    assert(SUCCEEDED(hr));

    return mipImages;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, const DirectX::TexMetadata& metadata) {
    D3D12_RESOURCE_DESC resourceDesc {};
    resourceDesc.Width = UINT(metadata.width);
    resourceDesc.Height = UINT(metadata.height);
    resourceDesc.MipLevels = UINT16(metadata.mipLevels);
    resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
    resourceDesc.Format = metadata.format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

    D3D12_HEAP_PROPERTIES heapProperties {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&resource)
    );
    assert(SUCCEEDED(hr));

    return resource;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages) {

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
    uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(mipImages.GetImageCount()));
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device.Get(), intermediateSize);
    UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());

    D3D12_RESOURCE_BARRIER barrier {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;

    commandList->ResourceBarrier(1, &barrier);
    return intermediateResource;
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += device->GetDescriptorHandleIncrementSize(type) * index;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += device->GetDescriptorHandleIncrementSize(type) * index;
    return handle;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height) {
    D3D12_RESOURCE_DESC resourceDesc {};
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.MipLevels = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProperties {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE depthClearValue {};
    depthClearValue.DepthStencil.Depth = 1.f;
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&resource)
    );
    assert(SUCCEEDED(hr));

    return resource;
}

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& fileName) {
    MaterialData materialData {};
    std::string line;
    std::ifstream file(directoryPath + "/" + fileName);
    assert(file.is_open());
    while (std::getline(file, line)){
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if(identifier == "map_Kd"){
            std::string textureFileName;
            s >> textureFileName;

            materialData.textureFilePath = directoryPath + "/" + textureFileName;
        }
    }
    return materialData;
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& fileName) {
    ModelData modelData {};
    std::vector<Vector4> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::string line;

    std::ifstream file(directoryPath + "/" + fileName);
    assert(file.is_open());

    while (std::getline(file, line)){
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if (identifier == "v"){
            Vector4 position {};
            s >> position.x >> position.y >> position.z;
            position.w = 1;
            positions.push_back(position);
        } else if (identifier == "vt"){
            Vector2 texcoord {};
            s >> texcoord.x >> texcoord.y;
            texcoords.push_back(texcoord);
        }else if (identifier == "vn"){
			Vector3 normal {};
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		} else if (identifier == "f"){
            VertexData triangle[3];
            for(int32_t faceVertex = 0; faceVertex < 3; ++faceVertex){
                std::string vertexDefinition;
                s >> vertexDefinition;

                std::istringstream v(vertexDefinition);
                uint32_t elementIndices[3];

                for(int32_t element = 0; element < 3; ++element){
                    std::string index;
                    std::getline(v, index, '/');
                    elementIndices[element] = std::stoi(index);
                }

                Vector4 position = positions[elementIndices[0] - 1];
                Vector2 texcoord = texcoords[elementIndices[1] - 1];
                Vector3 normal = normals[elementIndices[2] - 1];

                position.x *= -1;
                texcoord.y = 1 - texcoord.y;
                normal.x *= -1;

                triangle[faceVertex] = {position, texcoord, normal};
            }
            modelData.vertices.push_back(triangle[2]);
            modelData.vertices.push_back(triangle[1]);
            modelData.vertices.push_back(triangle[0]);
		}else if (identifier == "mtllib") {
            std::string materialFileName;
            s >> materialFileName;

            modelData.materialData = LoadMaterialTemplateFile(directoryPath, materialFileName);
        }
    }

    return modelData;
}

//Rect
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

Transform Camera {
    {1,1,1},
    {0,0,0},
    {0,0,-5}
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    CoInitializeEx(0, COINIT_MULTITHREADED);

    std::shared_ptr<D3DLeakChecker> leakChecker;

    Application* app = nullptr;
    app = new Application;
    app->Initialize(1280, 720);
    

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

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

    Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = {nullptr};

    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
    assert(SUCCEEDED(hr));

    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
    assert(SUCCEEDED(hr));

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = GetCPUHandle(device.Get(), rtvDescriptorHeap.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 0);
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
    D3D12_ROOT_PARAMETER rootParameter[4] = {};
    //Color
    rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameter[0].Descriptor.ShaderRegister = 0;
    //Transform
    rootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameter[1].Descriptor.ShaderRegister = 0;

    //DescriptorRange
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    //DescriptorTable
    rootParameter[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameter[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameter[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameter[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    //Lighting
    rootParameter[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameter[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameter[3].Descriptor.ShaderRegister = 1;

    descriptionRootSignature.pParameters = rootParameter;
    descriptionRootSignature.NumParameters = _countof(rootParameter);

    //Sampler
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

    if (FAILED(hr)){
        Log(static_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));

    //InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDescs[2].SemanticName = "NORMAL";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc {};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    //BlendState
    D3D12_BLEND_DESC blendDesc {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    //RasterizerState
    D3D12_RASTERIZER_DESC rasterizerDesc {};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    //Compile Shader
    Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(vertexShaderBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(pixelShaderBlob != nullptr);

    //DepthStencilState
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc {};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

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

    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

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

    //shader resource view
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

    //ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(
        device.Get(),
        swapChainDesc.BufferCount,
        rtvDesc.Format,
        srvDescriptorHeap.Get(),
        srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
    );

    //Input
    Input* input = nullptr;

	input = new Input();
    input->Initialize(wc.hInstance, hwnd);

#pragma region Triangle
    //Triangle
    //Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
    //vertexResource.Attach(CreateBufferResource(device.Get(), sizeof(VertexData) * 3 * 2));
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = (CreateBufferResource(device, sizeof(Material)));
    //Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource = nullptr;
    //transformationResource.Attach(CreateBufferResource(device.Get(), sizeof(TransformationMatrix)));
     
    //D3D12_VERTEX_BUFFER_VIEW vertexBufferView {};
    //vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    //vertexBufferView.SizeInBytes = sizeof(VertexData) * 3 * 2;
    //vertexBufferView.StrideInBytes = sizeof(VertexData);

    //VertexData* vertexData = nullptr;
    //vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    ////1枚目
    //vertexData[0].position = {-0.5f, -0.5f, 0.f, 1.f};
    //vertexData[1].position = {0.f, 0.5f, 0.f, 1.f};
    //vertexData[2].position = {0.5f, -0.5f, 0.f, 1.f};

    //vertexData[0].texcoord = {0, 1};
    //vertexData[1].texcoord = {0.5f, 0};
    //vertexData[2].texcoord = {1, 1};

    ////2枚目
    //vertexData[3].position = {-0.5f, -0.5f, 0.5f, 1};
    //vertexData[4].position = {0,0,0,1};
    //vertexData[5].position = {0.5f, -0.5f, -0.5f, 1};

    //vertexData[3].texcoord = {0,1};
    //vertexData[4].texcoord = {0.5f, 0};
    //vertexData[5].texcoord = {1,1};

    Material* materialData = nullptr;
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    materialData->color = {1, 1, 1, 1};
    materialData->enableLighting = true;
    materialData->uvTransform = MathUtils::Matrix::MakeIdentity();

    //TransformationMatrix* transformationData = nullptr;
    //transformationResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationData));
    //transformationData->WVP = MathUtils::Matrix::MakeIdentity();

    //Transform transform {
    //    {1,1,1},
    //    {0,0,0},
    //    {0,0,0}
    //};
    #pragma endregion

#pragma region Sprite
    /*
     * VertexResource
     * VertexBufferView
     * TransformMatrix用のCBV
     * CPUで扱うTransform
     */
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = (CreateBufferResource(device.Get(), sizeof(VertexData) * 4));
    D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite {};
    vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
    vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;
    vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = (CreateBufferResource(device.Get(), sizeof(uint32_t) * 6));

    D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite {};
    indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
    indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
    indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

    uint32_t* indexDataSprite = nullptr;
    indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));

    indexDataSprite[0] = 0;
    indexDataSprite[1] = 1;
    indexDataSprite[2] = 2;
    indexDataSprite[3] = 1;
    indexDataSprite[4] = 3;
    indexDataSprite[5] = 2;

    VertexData* vertexDataSprite = nullptr;
    vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

    vertexDataSprite[0].position = {0, 360, 0, 1};
    vertexDataSprite[1].position = {0, 0, 0, 1};
    vertexDataSprite[2].position = {640, 360, 0, 1};
    vertexDataSprite[3].position = {640, 0, 0, 1};

    vertexDataSprite[0].texcoord = {0, 1};
    vertexDataSprite[1].texcoord = {0, 0};
    vertexDataSprite[2].texcoord = {1, 1};
    vertexDataSprite[3].texcoord = {1, 0};

    vertexDataSprite[0].normal = {0,0,-1};
    vertexDataSprite[1].normal = {0,0,-1};
    vertexDataSprite[2].normal = {0,0,-1};
    vertexDataSprite[3].normal = {0,0,-1};

    //material
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = (CreateBufferResource(device.Get(), sizeof(Material)));

    Material* materialDataSprite = nullptr;
    materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
    materialDataSprite->color = {1,1,1,1};
    materialDataSprite->enableLighting = false;
    materialDataSprite->uvTransform = MathUtils::Matrix::MakeIdentity();

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite = (CreateBufferResource(device.Get(), sizeof(TransformationMatrix)));
    TransformationMatrix* transformationMatrixSprite = nullptr;
    transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixSprite));
    transformationMatrixSprite->WVP = MathUtils::Matrix::MakeIdentity();

    Transform transformSprite = {
        {1,1,1},
        {0,0,0},
        {0,0,0}
    };
    Transform uvTransformSprite {
        {1,1,1},
        {0,0,0},
        {0,0,0}
    };
#pragma endregion

#pragma region Sphere
    /*Sphere
    constexpr uint32_t kSubdivision = 16;
    const float kLatEvery = std::numbers::pi_v<float> / kSubdivision;
    const float kLonEvery = (2 * std::numbers::pi_v<float>) / kSubdivision;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSphere = nullptr;
	vertexResourceSphere.Attach(CreateBufferResource(device.Get(), sizeof(VertexData) * (kSubdivision * kSubdivision * 6)));

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere {};
	vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();
	vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * (kSubdivision * kSubdivision * 6);
	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSphere = nullptr;
	materialResourceSphere.Attach(CreateBufferResource(device.Get(), sizeof(Material)));

	Material* materialDataSphere = nullptr;
	materialResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSphere));
	materialDataSphere->color = {1, 1, 1, 1};
    materialDataSphere->enableLighting = true;
    materialDataSphere->uvTransform = MathUtils::Matrix::MakeIdentity();

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationResourceSphere = nullptr;
	transformationResourceSphere.Attach(CreateBufferResource(device.Get(), sizeof(TransformationMatrix)));

	VertexData* vertexDataSphere = nullptr;
	vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));

    for(uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex){
        float lat = -MathUtils::F_PI / 2.f + kLatEvery * float(latIndex);
        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex){
            float lon = float(lonIndex) * kLonEvery;

            uint32_t startIndex = (latIndex * kSubdivision + lonIndex) * 6;

            Vector4 a = {
                std::cosf(lat) * std::cosf(lon),
                std::sinf(lat),
                std::cosf(lat) * std::sinf(lon),
                1
            };

            Vector4 b = {
                std::cosf(lat + kLatEvery) * std::cosf(lon),
                std::sinf(lat + kLatEvery),
                std::cosf(lat + kLatEvery) * std::sinf(lon),
                1
            };

            Vector4 c = {
                std::cosf(lat) * std::cosf(lon + kLonEvery),
                std::sinf(lat),
                std::cosf(lat) * std::sinf(lon + kLonEvery),
                1
            };

            Vector4 d = {
                std::cosf(lat + kLatEvery) * std::cosf(lon + kLonEvery),
                std::sinf(lat + kLatEvery),
                std::cosf(lat + kLatEvery) * std::sinf(lon + kLonEvery),
                1
            };

            u = x, v = y
            u = lon, v = lat
            lon = x lat = y
            u = lonIndex / kSubdivision
            v = 1 - latIndex / kSubdivision

            vertexDataSphere[startIndex].position = a;
            vertexDataSphere[startIndex].texcoord = {
                static_cast<float>(lonIndex) / static_cast<float>(kSubdivision),
                1 - static_cast<float>(latIndex) / static_cast<float>(kSubdivision)
            };

            vertexDataSphere[startIndex].normal.x = vertexDataSphere[startIndex].position.x;
            vertexDataSphere[startIndex].normal.y = vertexDataSphere[startIndex].position.y;
            vertexDataSphere[startIndex].normal.z = vertexDataSphere[startIndex].position.z;

            vertexDataSphere[++startIndex].position = b;
            vertexDataSphere[startIndex].texcoord = {
                static_cast<float>(lonIndex) / static_cast<float>(kSubdivision),
                1 - static_cast<float>(latIndex + 1) / static_cast<float>(kSubdivision)
            };

        	vertexDataSphere[startIndex].normal.x = vertexDataSphere[startIndex].position.x;
            vertexDataSphere[startIndex].normal.y = vertexDataSphere[startIndex].position.y;
            vertexDataSphere[startIndex].normal.z = vertexDataSphere[startIndex].position.z;
            
        	vertexDataSphere[++startIndex].position = c;
            vertexDataSphere[startIndex].texcoord = {
                static_cast<float>(lonIndex + 1) / static_cast<float>(kSubdivision),
                1 - static_cast<float>(latIndex) / static_cast<float>(kSubdivision)
            };

            vertexDataSphere[startIndex].normal.x = vertexDataSphere[startIndex].position.x;
            vertexDataSphere[startIndex].normal.y = vertexDataSphere[startIndex].position.y;
            vertexDataSphere[startIndex].normal.z = vertexDataSphere[startIndex].position.z;

            vertexDataSphere[++startIndex].position = c;
            vertexDataSphere[startIndex].texcoord = {
            	static_cast<float>(lonIndex + 1) / static_cast<float>(kSubdivision),
            	1 - static_cast<float>(latIndex) / static_cast<float>(kSubdivision)
            };

            vertexDataSphere[startIndex].normal.x = vertexDataSphere[startIndex].position.x;
            vertexDataSphere[startIndex].normal.y = vertexDataSphere[startIndex].position.y;
            vertexDataSphere[startIndex].normal.z = vertexDataSphere[startIndex].position.z;

            vertexDataSphere[++startIndex].position = b;

            vertexDataSphere[startIndex].texcoord = {
            	static_cast<float>(lonIndex) / static_cast<float>(kSubdivision),
            	1 - static_cast<float>(latIndex + 1) / static_cast<float>(kSubdivision)
            };

            vertexDataSphere[startIndex].normal.x = vertexDataSphere[startIndex].position.x;
            vertexDataSphere[startIndex].normal.y = vertexDataSphere[startIndex].position.y;
            vertexDataSphere[startIndex].normal.z = vertexDataSphere[startIndex].position.z;

            vertexDataSphere[++startIndex].position = d;
            vertexDataSphere[startIndex].texcoord = {
	            static_cast<float>(lonIndex + 1) / static_cast<float>(kSubdivision),
	            1 - static_cast<float>(latIndex + 1) / static_cast<float>(kSubdivision)
            };

            vertexDataSphere[startIndex].normal.x = vertexDataSphere[startIndex].position.x;
            vertexDataSphere[startIndex].normal.y = vertexDataSphere[startIndex].position.y;
            vertexDataSphere[startIndex].normal.z = vertexDataSphere[startIndex].position.z;
	    }
    }

	TransformationMatrix* transformationDataSphere = nullptr;
	transformationResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&transformationDataSphere));
	transformationDataSphere->WVP = MathUtils::Matrix::MakeIdentity();

	Transform transformSphere {
		{1,1,1},
		{0,0,0},
		{0,0,0}
	};*/
#pragma endregion

#pragma region Model
    ModelData modelData = LoadObjFile("resources", "axis.obj");
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = (CreateBufferResource(device.Get(), sizeof(VertexData)* modelData.vertices.size()));

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView {};
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData)* modelData.vertices.size());

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource = (CreateBufferResource(device.Get(), sizeof(TransformationMatrix)));

    TransformationMatrix* transformationData = nullptr;
    transformationResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationData));

    Transform modelTransform {
        {1,1,1},
        {0,0,0},
        {0,0,0}
    };
#pragma endregion

    //texture
    DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = (CreateTextureResource(device.Get(), metadata));
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = (UploadTextureData(device.Get(), commandList.Get(), textureResource.Get(), mipImages));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc {};
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUHandle(device.Get(), srvDescriptorHeap.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUHandle(device.Get(), srvDescriptorHeap.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

	//2枚目
    DirectX::ScratchImage mipImages2 = LoadTexture(modelData.materialData.textureFilePath);
    const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(device.Get(), metadata2);
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource2 = (UploadTextureData(device.Get(), commandList.Get(), textureResource2.Get(), mipImages2));

	device->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 {};
    srvDesc2.Format = metadata2.format;
    srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUHandle(device.Get(), srvDescriptorHeap.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUHandle(device.Get(), srvDescriptorHeap.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);

    device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);

    bool useMonsterBall = true;

    //DepthStencilTexture
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = (CreateDepthStencilResource(device.Get(), kClientWidth, kClientHeight));

    //DepthStencilView
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = (CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    //Light
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource = (CreateBufferResource(device.Get(), sizeof(DirectionalLight)));

    DirectionalLight* directionalLight = nullptr;

    directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLight));

	directionalLight->color = {1,1,1,1};
    directionalLight->direction = {0, -1, 0};
    directionalLight->intensity = 1;

#pragma region MainLoop
    MSG msg {};
    while(msg.message != WM_QUIT){
        if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }else{
            //do somethings...//

            //BeginFrame
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            ImGui::ShowDemoWindow();
#pragma region Update
            //Input
            input->Update();

            //Triangle
            /*transform.rotate.y += 0.01f;*/
            Matrix4x4 cameraMatrix = MathUtils::Matrix::MakeAffineMatrix(Camera.scale, Camera.rotate, Camera.translate);
            Matrix4x4 viewMatrix = cameraMatrix.Inverse();
            Matrix4x4 projectionMatrix = MathUtils::Matrix::MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100);
            /*Matrix4x4 wvp = MathUtils::Matrix::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate) * viewMatrix * projectionMatrix;
            transformationData->WVP = wvp;*/

            //Sphere
            //transformationDataSphere->World = MathUtils::Matrix::MakeAffineMatrix(transformSphere.scale, transformSphere.rotate, transformSphere.translate);
            //Matrix4x4 wvp = transformationDataSphere->World * (viewMatrix * projectionMatrix);
            //transformationDataSphere->WVP = wvp;

            //ImGui::Begin("Sphere");
            //ImGui::DragFloat3("Rotate", &transformSphere.rotate.x, 0.1f);
            //ImGui::End();


            //Model
            ImGui::Begin("Model");
            ImGui::DragFloat3("Transform", &modelTransform.translate.x, 0.01f);
            ImGui::SliderAngle("Rotate.X", &modelTransform.rotate.x, -360, 360);
            ImGui::SliderAngle("Rotate.Y", &modelTransform.rotate.y, -360, 360);
            ImGui::SliderAngle("Rotate.Z", &modelTransform.rotate.z, -360, 360);
            ImGui::End();

            transformationData->World = MathUtils::Matrix::MakeAffineMatrix(modelTransform.scale, modelTransform.rotate, modelTransform.translate);
            transformationData->WVP = transformationData->World * viewMatrix * projectionMatrix;

            //Sprite
            ImGui::Begin("Sprite Transform");
            ImGui::DragFloat2("Scale", &transformSprite.scale.x, 0.1f);
            ImGui::DragFloat2("Translate", &transformSprite.translate.x, 1);
            ImGui::DragFloat2("uvTranslate", &uvTransformSprite.translate.x, 0.1f);
            ImGui::DragFloat2("uvScale", &uvTransformSprite.scale.x, 0.01f);
            ImGui::SliderAngle("uvRotate", &uvTransformSprite.rotate.z, -360, 360);
            ImGui::End();
            Matrix4x4 viewMatrixSprite = MathUtils::Matrix::MakeIdentity();
            Matrix4x4 projectionMatrixSprite = MathUtils::Matrix::MakeOrthogonalMatrix(0, float(kClientWidth), 0, float(kClientHeight), 0, 100);
            transformationMatrixSprite->World = MathUtils::Matrix::MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
            Matrix4x4 worldViewProjectionMatrixSprite = transformationMatrixSprite->World * viewMatrixSprite * projectionMatrixSprite;
            transformationMatrixSprite->WVP = worldViewProjectionMatrixSprite;

            Matrix4x4 uvTransformMatrix = MathUtils::Matrix::MakeScaleMatrix(uvTransformSprite.scale);
            uvTransformMatrix = uvTransformMatrix * MathUtils::Matrix::MakeRotateZ(uvTransformSprite.rotate.z);
            uvTransformMatrix = uvTransformMatrix * MathUtils::Matrix::MakeTranslateMatrix(uvTransformSprite.translate);
            materialDataSprite->uvTransform = uvTransformMatrix;


            // texture
            ImGui::Begin("Texture");
            ImGui::Checkbox("Use MonsterBall", &useMonsterBall);
            ImGui::End();

            ImGui::Begin("Light");
            ImGui::DragFloat3("Direction", &directionalLight->direction.x, 0.01f);
            ImGui::End();

            directionalLight->direction.normalize();

#pragma endregion

            ImGui::Render();

            UINT bbi = swapChain->GetCurrentBackBufferIndex();

            D3D12_RESOURCE_BARRIER barrier {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = swapChainResources[bbi].Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

            commandList->ResourceBarrier(1, &barrier);

            D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            commandList->OMSetRenderTargets(1, &rtvHandles[bbi], false, &dsvHandle);

            float color[4] = {0.1f, 0.25f, 0.5f, 1};
            commandList->ClearRenderTargetView(rtvHandles[bbi], color, 0, nullptr);
            commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeaps[] = {srvDescriptorHeap};
            commandList->SetDescriptorHeaps(1, descriptorHeaps->GetAddressOf());


            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            commandList->SetGraphicsRootSignature(rootSignature.Get());

            commandList->SetPipelineState(graphicsPipelineState.Get());

#pragma region Draw
            //Input
            if(input->PushKey(DIK_A)){
                Log("Hit - A\n");
            }

            //Triangle
            /*commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);*/
            commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
            //commandList->SetGraphicsRootConstantBufferView(1, transformationResource->GetGPUVirtualAddress());
            commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU2);
            //commandList->DrawInstanced(3 * 2, 1, 0, 0 );

            //Sphere
            //commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);
            //commandList->SetGraphicsRootConstantBufferView(0, materialResourceSphere->GetGPUVirtualAddress());
            commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            //commandList->SetGraphicsRootConstantBufferView(1, transformationResourceSphere->GetGPUVirtualAddress());
            //commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
            commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
            /*commandList->DrawInstanced(kSubdivision * kSubdivision * 6, 1, 0, 0);*/

            //Model
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            commandList->SetGraphicsRootConstantBufferView(1, transformationResource->GetGPUVirtualAddress());
            commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

            //Sprite
            /*commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
            commandList->IASetIndexBuffer(&indexBufferViewSprite);
            commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
            commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
        	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);*/

#pragma endregion

            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

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

#pragma endregion

    delete input;
    delete app;

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CloseHandle(fenceEvent);
    CloseWindow(hwnd);
    CoUninitialize();

    return 0;
}

