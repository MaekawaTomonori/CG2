#include "Sprite.h"

#include "CommandController.h"
#include "DeviceManager.h"
#include "MathUtils.h"
#include "Shader.h"

void Sprite::Initialize() {
    //四角だが、2枚の三角形を組み合わせて四角として扱う
    vertexResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(VertexData) * 6);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    //頂点情報の初期化
    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    //1枚目の三角形
    vertexData[0].position = {0, 360, 0, 1};
    vertexData[0].texCoord = {0, 1};

    vertexData[1].position = {0,0,0,1};
    vertexData[1].texCoord = {0,0};

    vertexData[2].position = {640, 360, 0, 1};
    vertexData[2].texCoord = {1,1};

	//2枚目の三角形
    vertexData[3].position = {0, 0, 0, 1};
    vertexData[3].texCoord = {0, 0};

    vertexData[4].position = {640,0,0,1};
    vertexData[4].texCoord = {1,0};

    vertexData[5].position = {640, 360, 0, 1};
    vertexData[5].texCoord = {1,1};

    transformationMatrixResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(Matrix4x4));
    
	transformMatrix_ = nullptr;
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformMatrix_));
    *transformMatrix_ = MathUtils::Matrix::MakeIdentity();
    transform_ = {{1,1,1,}, {0,0,0},{0,0,0}};
    Matrix4x4 wvp = MathUtils::Matrix::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate)*MathUtils::Matrix::MakeIdentity() * MathUtils::Matrix::MakeOrthogonalMatrix(0,0, 1280, 720, 0,100);
    *transformMatrix_ = wvp;
}

void Sprite::Update() {
	Matrix4x4 worldMatrix = MathUtils::Matrix::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 viewMatrix = MathUtils::Matrix::MakeIdentity();
    Matrix4x4 projectionMatrix = MathUtils::Matrix::MakeOrthogonalMatrix(0, float(CLIENT_WIDTH), 0, float(CLIENT_HEIGHT), 0, 100.f);
    Matrix4x4 worldViewProjectionMatrix = worldMatrix * (viewMatrix * projectionMatrix);
    *transformMatrix_ = worldViewProjectionMatrix;
}

void Sprite::Draw() {
    Singleton<CommandController>::getInstance()->getList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
    Singleton<CommandController>::getInstance()->getList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

    if(textureHandle_){
        Singleton<CommandController>::getInstance()->getList()->SetGraphicsRootDescriptorTable(2, textureHandle_.get_value());
    }

    Singleton<CommandController>::getInstance()->getList()->DrawInstanced(6, 1, 0, 0);
}

void Sprite::Initialize(D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
    textureHandle_ = textureHandle;
    Initialize();
}
