#pragma once
#include <wrl/client.h>

#include "Singleton.h"

class DeviceManager final : Singleton<DeviceManager>{
    friend class Singleton<DeviceManager>;

    DeviceManager() {
        System::Debug::Log(System::Debug::ConvertString(L"DeviceManager Enable\n"));
    }
    ~DeviceManager() {
        System::Debug::Log(System::Debug::ConvertString(L"DeviceManager Disable\n"));
    }

    //Members
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;

public:
    Microsoft::WRL::ComPtr<ID3D12Device> getDevice() const {
    	return device_;
    }
    Microsoft::WRL::ComPtr<IDXGIFactory7> getDXGIFactory()const {
        return dxgiFactory_;
    }

    void RegisteringDevice();
};
