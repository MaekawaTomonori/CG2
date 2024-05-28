#pragma once
#include "Nullable.h"
#include "Object.h"

class Sprite :
    public Object{
    Color color_{1,1,1,1};
    nullable<D3D12_GPU_DESCRIPTOR_HANDLE> textureHandle_;

    Matrix4x4* transformMatrix_ = nullptr;
public:
    void Initialize() override;
    void Update() override;
    void Draw() override;

    void Initialize(D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);
    void Initialize(Color color);
};

