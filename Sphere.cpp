#include "Sphere.h"

#include "CommandController.h"
#include "DeviceManager.h"
#include "Shader.h"

void Sphere::Initialize() {
    vertexResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(VertexData) * SUBDIVISION * SUBDIVISION * 6);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * (SUBDIVISION * SUBDIVISION * 6);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    for(uint32_t latIndex = 0; latIndex < SUBDIVISION; ++latIndex){
        float lat = -MathUtils::F_PI / 2.f + LAT_EVERY * latIndex;
	    for (uint32_t lonIndex = 0; lonIndex < SUBDIVISION; ++lonIndex){
            float lon = lonIndex * LON_EVERY;

            uint32_t startIndex = (latIndex * SUBDIVISION + lonIndex) * 6;

            Vector4 a = {cosf(lat) * cosf(lon), sin(lat), cosf(lat) * sinf(lon), 1};
            Vector4 b = {cosf(lat - LAT_EVERY) * cosf(lon), sinf(lat - LAT_EVERY), cosf(lat - LAT_EVERY) * sinf(lon),1};
            Vector4 c = {cosf(lat) * cosf(lon + LON_EVERY), sinf(lat), cosf(lat) * sinf(lon + LON_EVERY), 1};
            Vector4 d = {
	            cosf(lat + LAT_EVERY) * cosf(lon + LON_EVERY), sinf(lat + LAT_EVERY),
	            cosf(lat + LAT_EVERY) * sinf(lon + LON_EVERY),1
            };

            float u = static_cast<float>(lonIndex) / SUBDIVISION;
            float v = 1.f - static_cast<float>(latIndex) / SUBDIVISION;

            vertexData[startIndex].position = a;
            vertexData[startIndex].texCoord = {u, v};
            vertexData[++startIndex].position = b;
            vertexData[startIndex].texCoord = {u, v};
            vertexData[++startIndex].position = c;
            vertexData[startIndex].texCoord = {u, v};

            vertexData[++startIndex].position = c;
            vertexData[startIndex].texCoord = {u, v};
            vertexData[++startIndex].position = b;
            vertexData[startIndex].texCoord = {u, v};
            vertexData[++startIndex].position = d;
            vertexData[startIndex].texCoord = {u, v};
	    }
    }

    transform_ = {
        {1,1,1},
        {0,0,0},
        {0,0,0}
    };

    transformationMatrixResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(Matrix4x4));
    transformationMatrixData = nullptr;
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(transformationMatrixData.get()));
    *transformationMatrixData = MathUtils::Matrix::MakeIdentity();
}

void Sphere::Update() {
    Matrix4x4 worldMatrix = MathUtils::Matrix::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 viewMatrix = MathUtils::Matrix::MakeIdentity();
    Matrix4x4 projectionMatrix = MathUtils::Matrix::MakeOrthogonalMatrix(0, float(CLIENT_WIDTH), 0, float(CLIENT_HEIGHT), 0, 100.f);
    Matrix4x4 worldViewProjectionMatrix = worldMatrix * (viewMatrix * projectionMatrix);
    *transformationMatrixData = worldViewProjectionMatrix;
}

void Sphere::Draw() {
    Singleton<CommandController>::getInstance()->getList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
    Singleton<CommandController>::getInstance()->getList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());
    if (textureHandle_){
    	Singleton<CommandController>::getInstance()->getList()->SetGraphicsRootDescriptorTable(2, textureHandle_.get_value());
    }
    Singleton<CommandController>::getInstance()->getList()->DrawInstanced(SUBDIVISION*SUBDIVISION*6, 1, 0, 0);
}

void Sphere::Initialize(D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
    textureHandle_ = textureHandle;
    Initialize();
}
