#pragma once
#include "Object.h"

struct DirectionalLight{
    Color color;//!<色
    Vector3 direction;//!<向き
    float intensity;//!<輝度
};

class Light : Singleton<Light>{
    friend Singleton <Light>;

    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLight* directionalLight_;

    Light() {
        System::Debug::Log(System::Debug::ConvertString(L"Light Enabled\n"));
    }
    ~Light() {
        directionalLightResource_->Release();
        System::Debug::Log(System::Debug::ConvertString(L"Light Disabled\n"));
    }

public:
    void registerDirectionalLight();
    Microsoft::WRL::ComPtr<ID3D12Resource> getDirectionalLight() {
	    return directionalLightResource_;
    }
    DirectionalLight* getDirectionalLightData() const {
        return directionalLight_;
    }
};