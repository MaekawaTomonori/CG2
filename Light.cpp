#include "Light.h"

#include "DeviceManager.h"
#include "Shader.h"

void Light::registerDirectionalLight() {
    directionalLightResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(DirectionalLight));
    directionalLight_ = nullptr;
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLight_));
    directionalLight_->color = {1,1,1,1};
    directionalLight_->direction = {0, -1, 0};
    directionalLight_->intensity = 1;
    System::Debug::Log(System::Debug::ConvertString(L"DirectionalLight Enabled\n"));
}
