#pragma once

namespace System{
    namespace Debug {
        std::wstring ConvertString(const std::string& str);

        std::string ConvertString(const std::wstring& str);

        void Log(const std::string& message);

        void Log(const std::wstring& message);
    }

    IDxcBlob* CompileShader(const std::wstring& filePath,
                            const wchar_t* profile,
                            IDxcUtils* dxcUtils,
                            IDxcCompiler3* dxcCompiler,
                            IDxcIncludeHandler* includeHandler);
};

