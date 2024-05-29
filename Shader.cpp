#include "Shader.h"

///create vertex resource
ID3D12Resource* Shader::CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
    //VertexResource Heap setting
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
    HRESULT hR = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &materialResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource));

    assert(SUCCEEDED(hR));

    return materialResource;
}
IDxcBlob* Shader::CompileShader(const std::wstring& filePath, const wchar_t* profile, IDxcUtils* dxcUtils,
                                IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler) {
    System::Debug::Log(System::Debug::ConvertString(std::format(L"Begin CompileShader, Path : {}, Profile : {}\n", filePath, profile)));
    IDxcBlobEncoding* shaderSource = nullptr;
    HRESULT hResult = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    assert(SUCCEEDED(hResult));

    DxcBuffer shaderSourceBuffer;
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8;

    ///Compiling
    LPCWSTR arguments[] = {
        filePath.c_str(), //対象のhlslファイル名
        L"-E", L"main", //EntryPoint
        L"-T", profile, //ShaderProfile
        L"-Zi", L"-Qembed_debug", //DebugInfo
        L"-Od", //最適化を外す
        L"-Zpr", //メモリレイアウトは行優先
    };

    IDxcResult* shaderResult = nullptr;
    hResult = dxcCompiler->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler,
        IID_PPV_ARGS(&shaderResult)
    );
    assert(SUCCEEDED(hResult));

    IDxcBlobUtf8* shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if(shaderError != nullptr && shaderError->GetStringLength() != 0){
        System::Debug::Log(shaderError->GetStringPointer());

        assert(false);
    }

    IDxcBlob* shaderBlob = nullptr;
    hResult = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hResult));

    System::Debug::Log(System::Debug::ConvertString(std::format(L"Compile Succeed, Path : {}, Profile : {}\n", filePath, profile)));
    shaderSource->Release();
    shaderResult->Release();

    return shaderBlob;
}