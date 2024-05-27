#include "DeviceManager.h"

void DeviceManager::RegisteringDevice() {
    if(device_){return;}

    HRESULT hResult = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));

    assert(SUCCEEDED(hResult));


    for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) != DXGI_ERROR_NOT_FOUND; ++i){
        DXGI_ADAPTER_DESC3 adapterDesc {};
		hResult = useAdapter_->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hResult));

        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)){
            System::Debug::Log(std::format(L"Use Adapter:{}\n", adapterDesc.Description));
            break;
        }
        useAdapter_ = nullptr;
    }

    assert(useAdapter_ != nullptr);

    //==//
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
    };
    const char* featureLevelStrings[] = {"12.2", "12.1", "12.0"};
    
    for (size_t i = 0; i < _countof(featureLevels); ++i){
        hResult = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));

        if (SUCCEEDED(hResult)){
            System::Debug::Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
            break;
        }
    }

    assert(device_ != nullptr);
    System::Debug::Log(System::Debug::ConvertString(std::format(L"Complete create D3D12Device!!!\n")));
}
