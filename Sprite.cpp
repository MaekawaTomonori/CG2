#include "Sprite.h"

#include "CommandController.h"
#include "DeviceManager.h"
#include "Light.h"
#include "MathUtils.h"
#include "Shader.h"

void Sprite::Initialize() {
    System::Debug::Log(System::Debug::ConvertString(L"[Sprite] : Initializing...\n"));
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
    vertexData[0].normal = {0, 0, -1};

    vertexData[1].position = {0,0,0,1};
    vertexData[1].texCoord = {0,0};
    vertexData[1].normal = {0, 0, -1};

    vertexData[2].position = {640, 360, 0, 1};
    vertexData[2].texCoord = {1,1};
    vertexData[2].normal = {0, 0, -1};

	//2枚目の三角形
    vertexData[3].position = {0, 0, 0, 1};
    vertexData[3].texCoord = {0, 0};
    vertexData[3].normal = {0, 0, -1};

    vertexData[4].position = {640,0,0,1};
    vertexData[4].texCoord = {1,0};
    vertexData[4].normal = {0, 0, -1};

    vertexData[5].position = {640, 360, 0, 1};
    vertexData[5].texCoord = {1,1};
    vertexData[5].normal = {0, 0, -1};

    materialResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice(), sizeof(Material));
    //color
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&material_));
    *material_.color = {1,1,1,1};
    //Lightingを無効化
    material_.enableLighting = false;

    transformationMatrixResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(TransformationMatrix));
    
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrix_));
    transformationMatrix_.WVP = MathUtils::Matrix::MakeIdentity();
    transformationMatrix_.World = MathUtils::Matrix::MakeIdentity();

    transform_ = {{1,1,1,}, {0,0,0},{0,0,0}};
    transformationMatrix_.World = MathUtils::Matrix::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    transformationMatrix_.WVP = transformationMatrix_.World * MathUtils::Matrix::MakeIdentity() * MathUtils::Matrix::MakeOrthogonalMatrix(0, float(CLIENT_WIDTH), 0, float(CLIENT_HEIGHT), 0, 100);

    System::Debug::Log(System::Debug::ConvertString(L"[Sprite] : Initialized!\n"));
}

void Sprite::Update() {
    /*
     * ImGui
     */

    /*ImGui::Begin("SpriteDebug");
    ImGui::SliderFloat4("Color", &color_->x, 0, 1);
    ImGui::End();*/

    /*
     * Update
     */
	transformationMatrix_.World = MathUtils::Matrix::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 viewMatrix = MathUtils::Matrix::MakeIdentity();
    Matrix4x4 projectionMatrix = MathUtils::Matrix::MakeOrthogonalMatrix(0, float(CLIENT_WIDTH), 0, float(CLIENT_HEIGHT), 0.1f, 100.f);
    Matrix4x4 worldViewProjectionMatrix = transformationMatrix_.World * (viewMatrix * projectionMatrix);
    transformationMatrix_.WVP = worldViewProjectionMatrix;
}

void Sprite::Draw() {
    Singleton<CommandController>::getInstance()->getList()->IASetVertexBuffers(0, 1, &vertexBufferView_);

    Singleton<CommandController>::getInstance()->getList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
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

void Sprite::Initialize(Color& color) {
	this->material_.color = &color;
    Initialize();
}

void Sprite::EditParameterByImGui() {
}
