#pragma once
#include <wrl/client.h>

#include "Nullable.h"

//struct Color{
//    float red;
//    float green;
//    float blue;
//    float alpha;
//};

typedef Vector4 Color;

class Object{
    // ==Objectに必要なもの==
    // VertexResource
    // VertexBufferView (VBV)
    // TransformMatrix用のCBV
    // CPUで扱うTransform
protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    //リソースにマップするもの
    Matrix4x4* transformationMatrixData = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    Transform transform_{};

    //なにも指定されなかった場合White1x1が入る設定にする
    nullable<D3D12_GPU_DESCRIPTOR_HANDLE> textureHandle_;

    //初期値は白 リソースにマッピングするので生ぽでOK
    Color* color_ = new Color(1.f,1.f,1.f,1.f);

    //識別
    std::string uuid_{};

public:
	Object() = default;
    virtual ~Object() = default;
    virtual void Initialize() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;
};

