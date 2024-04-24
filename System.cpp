#include "System.h"

#include <format>

std::wstring System::Debug::ConvertString(const std::string& str) {
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

std::string System::Debug::ConvertString(const std::wstring& str) {
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

void System::Debug::Log(const std::string& message) {
    OutputDebugStringA(message.c_str());
}

void System::Debug::Log(const std::wstring& message) {
    OutputDebugStringA(ConvertString(message).c_str());
}

IDxcBlob* System::CompileShader(const std::wstring& filePath, const wchar_t* profile, IDxcUtils* dxcUtils,
                                IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler) {
    Debug::Log(Debug::ConvertString(std::format(L"Begin CompileShader, Path : {}, Profile : {}\n", filePath, profile)));
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
        Debug::Log(shaderError->GetStringPointer());

        assert(false);
    }

    IDxcBlob* shaderBlob = nullptr;
    hResult = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hResult));

    Debug::Log(Debug::ConvertString(std::format(L"Compile Succeed, Path : {}, Profile : {}")));
    shaderSource->Release();
    shaderResult->Release();

    return shaderBlob;
}
