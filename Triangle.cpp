#include "Triangle.h"

#include "CommandController.h"
#include "DeviceManager.h"
#include "MathUtils.h"
#include "Shader.h"

Triangle::~Triangle() {
    materialResource_->Release();

    System::Debug::Log(System::Debug::ConvertString(L"[Triangle] : Delete\n"));
}

void Triangle::Initialize() {
    System::Debug::Log(System::Debug::ConvertString(L"[Triangle] : Initializing...\n"));
    transform_ = {
        {1,1,1},
        {0,0,0},
        {0,0,0}
    };

    vertexResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(VertexData) * 3);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * 3;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    //LeftBtm
    vertexData[0].position = {-0.5f, -0.5f, 0, 1};
    vertexData[0].texCoord = {0, 1};

    //Top
    vertexData[1].position = {0, 0.5f, 0, 1};
    vertexData[1].texCoord = {0.5f, 0};

    //RightBtm
    vertexData[2].position = {0.5f, -0.5f, 0, 1};
    vertexData[2].texCoord = {1, 1};

    transformationMatrixResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(Matrix4x4));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
    *transformationMatrixData = MathUtils::Matrix::MakeIdentity();

    materialResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(VertexData) * 3);
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&color_));
    *color_ = {1,1,1,1};

    System::Debug::Log(System::Debug::ConvertString(L"[Triangle] : Initialized!\n"));
}

void Triangle::Update() {
    transform_.rotate.y += 0.01f;
    worldMatrix = MathUtils::Matrix::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
    cameraMatrix = MathUtils::Matrix::MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
    viewMatrix = cameraMatrix.Inverse();
    projectionMatrix = MathUtils::Matrix::MakePerspectiveFovMatrix(0.45f, static_cast<float>(CLIENT_HEIGHT) / static_cast<float>(CLIENT_WIDTH), 0.1f, 100.f);
    worldViewProjectionMatrix = worldMatrix * (viewMatrix * projectionMatrix);
    *transformationMatrixData = worldViewProjectionMatrix;
    *transformationMatrixData = worldMatrix;

    ImGui::Begin("Triangle");
    ImGui::SliderFloat4("color", &color_->x, 0, 1);
    ImGui::End();
}

void Triangle::Draw() {
	Singleton<CommandController>::getInstance()->getList().Get()->IASetVertexBuffers(0, 1, &vertexBufferView_);
    Singleton<CommandController>::getInstance()->getList().Get()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Singleton<CommandController>::getInstance()->getList().Get()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    Singleton<CommandController>::getInstance()->getList().Get()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());
    Singleton<CommandController>::getInstance()->getList().Get()->DrawInstanced(3, 1, 0, 0);
}
