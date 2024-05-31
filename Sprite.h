#pragma once
#include "Object.h"

class Sprite :
    public Object{

    std::unique_ptr<Matrix4x4> transformMatrix_ = nullptr;
public:
    void Initialize() override;
    void Update() override;
    void Draw() override;

    void Initialize(D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);
    void Initialize(Color& color);
};

