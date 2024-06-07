#pragma once
#include <wrl/client.h>

#include "Nullable.h"

#include <rpc.h>

#pragma comment(lib, "Rpcrt4.lib")

//struct Color{
//    float red;
//    float green;
//    float blue;
//    float alpha;
//};

typedef Vector4 Color;

struct Material{
    //初期値は白 リソースにマッピングするので生ぽでOK
    Color color;
    int32_t enableLighting;
};

struct TransformationMatrix{
    Matrix4x4 WVP;
    Matrix4x4 World;
};

class Object{
    // ==Objectに必要なもの==
    // VertexResource
    // VertexBufferView (VBV)
    // TransformMatrix用のCBV
    // CPUで扱うTransform
protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

    //リソースにマップするもの
    TransformationMatrix* transformationMatrix_ = nullptr;
    Material* material_ = nullptr;

    Transform transform_{};

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_ {};

    //なにも指定されなかった場合White1x1が入る設定にする
    std::string textureName_;
    nullable<D3D12_GPU_DESCRIPTOR_HANDLE> textureHandle_;


    //識別
    std::string uuid_{};

public:
	Object() {
        UUID uuid;
        UuidCreate(&uuid);
        RPC_CSTR szUuid = nullptr;
        UuidToStringA(&uuid, &szUuid);
        uuid_ = (char*)szUuid;
        RpcStringFreeA(&szUuid);
	}
    virtual ~Object() = default;
    virtual void Initialize() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;

    void changeTexture(const std::string& textureName) {
        //同じもの突っ込んだら追い返す
        if (textureName_ == textureName)return;
        textureName_ = textureName;
        textureHandle_ = Singleton<TextureManager>::getInstance()->GetRegisteredTextures().at(textureName).get()->GetHandle();
    }

private:
    virtual void EditParameterByImGui() = 0;
};

