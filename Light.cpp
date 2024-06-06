#include "Light.h"

#include "DeviceManager.h"
#include "Shader.h"

void Light::registerDirectionalLight() {
    directionalLightResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(DirectionalLight));
    DirectionalLight directionalLight {};
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLight));
    directionalLight.color = {1,1,1,1};
    directionalLight.direction = {0, -1, 0};
    directionalLight.intensity = 1;
    System::Debug::Log(System::Debug::ConvertString(L"DirectionalLight Enabled\n"));
}
