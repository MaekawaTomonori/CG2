#pragma once

struct Color{
    float red;
    float green;
    float blue;
    float alpha;
};

class Object{
    // ==Objectに必要なもの==
    // VertexResource
    // VertexBufferView (VBV)
    // TransformMatrix用のCBV
    // CPUで扱うTransform
protected:
	ID3D12Resource* vertexResource_ = nullptr;
    ID3D12Resource* transformationMatrixResource_ = nullptr;
    Matrix4x4* transformationMatrixData = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    Transform transform_{};

public:
	Object() = default;
    virtual ~Object() = default;
    virtual void Initialize() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;
    virtual void Release() {
        vertexResource_->Release();
        transformationMatrixResource_->Release();
    }
};

