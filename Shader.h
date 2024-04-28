#pragma once
class Shader{
public:
    static ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);
    static IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile, IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler);
};

