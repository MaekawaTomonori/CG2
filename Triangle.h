#pragma once
#include "Object.h"
class Triangle :
    public Object{
    Transform transform {{1.f,1.f,1.f}, {0.f,0.f,0.f}, {0.f,0.f,0.f}},
        cameraTransform {{1,1,1}, {0,0,0}, {0,0,-5}};
    Matrix4x4 worldMatrix {}, cameraMatrix {}, viewMatrix {}, projectionMatrix {}, worldViewProjectionMatrix {};
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
public:
	~Triangle() override;
    void Initialize() override;
    void Update() override;
    void Draw() override;
};

