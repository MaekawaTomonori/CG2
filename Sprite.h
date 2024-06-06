#pragma once
#include "Object.h"

class Sprite :
    public Object{

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
public:
    void Initialize() override;
    void Update() override;
    void Draw() override;

    void Initialize(D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);
    void Initialize(Color& color);

private:
    void EditParameterByImGui() override;
};

