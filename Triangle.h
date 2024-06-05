#pragma once
#include "Object.h"
class Triangle final :
    public Object{
    Transform cameraTransform {{1,1,1}, {0,0,0}, {0,0,-5}};
    Matrix4x4 worldMatrix {}, cameraMatrix {}, viewMatrix {}, projectionMatrix {}, worldViewProjectionMatrix {};
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
public:
	//~Triangle() ;
    void Initialize() override;
    void Update() override;
    void Draw() override;
    void EditParameterByImGui() override;

    void setTransform(const Transform& t) {
        transform_ = t;
    }
};

