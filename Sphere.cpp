#include "Sphere.h"

#include "CommandController.h"
#include "DeviceManager.h"
#include "Light.h"
#include "Shader.h"

void Sphere::Initialize() {
    vertexResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(VertexData) * SUBDIVISION * SUBDIVISION * 6);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * (SUBDIVISION * SUBDIVISION * 6);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    materialResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&material_));
    material_->color = {1,1,1,1};
	material_->enableLighting = true;

    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    for(uint32_t latIndex = 0; latIndex < SUBDIVISION; ++latIndex){
        float lat = -MathUtils::F_PI / 2.f + LAT_EVERY * static_cast<float>(latIndex);
	    for (uint32_t lonIndex = 0; lonIndex < SUBDIVISION; ++lonIndex){
            float lon = static_cast<float>(lonIndex) * LON_EVERY;

            //lat(緯度/縦) = θ, lon(経度/横) = φ

            uint32_t startIndex = (latIndex * SUBDIVISION + lonIndex) * 6;

            Vector4 a = {
            	cosf(lat) * cosf(lon),
            	sin(lat),
            	cosf(lat) * sinf(lon),
            	1
            };
            Vector4 b = {
            	cosf(lat + LAT_EVERY) * cosf(lon),
            	sinf(lat + LAT_EVERY),
            	cosf(lat + LAT_EVERY) * sinf(lon),
            	1
            };
            Vector4 c = {
            	cosf(lat) * cosf(lon + LON_EVERY),
            	sinf(lat),
            	cosf(lat) * sinf(lon + LON_EVERY),
            	1
            };
            Vector4 d = {
	            cosf(lat + LAT_EVERY) * cosf(lon + LON_EVERY),
            	sinf(lat + LAT_EVERY),
	            cosf(lat + LAT_EVERY) * sinf(lon + LON_EVERY),
            	1
            };

            //基になる考え方
            // u = x, v = y
            //u = lonIndex / SUBDIVISION;
            //v = 1.f - latIndex / SUBDIVISION;

            vertexData[startIndex].position = a;
            vertexData[startIndex].texCoord = {static_cast<float>(lonIndex) / static_cast<float>(SUBDIVISION), 1 - static_cast<float>(latIndex)/static_cast<float>(SUBDIVISION)};

            vertexData[startIndex].normal.x = vertexData[startIndex].position.x;
            vertexData[startIndex].normal.y = vertexData[startIndex].position.y;
            vertexData[startIndex].normal.z = vertexData[startIndex].position.z;

            vertexData[++startIndex].position = b;
            vertexData[startIndex].texCoord = {static_cast<float>(lonIndex) / static_cast<float>(SUBDIVISION), 1 - static_cast<float>(latIndex + 1) / static_cast<float>(SUBDIVISION)};

            vertexData[startIndex].normal.x = vertexData[startIndex].position.x;
            vertexData[startIndex].normal.y = vertexData[startIndex].position.y;
            vertexData[startIndex].normal.z = vertexData[startIndex].position.z;

	    	vertexData[++startIndex].position = c;
            vertexData[startIndex].texCoord = {static_cast<float>(lonIndex + 1) / static_cast<float>(SUBDIVISION), 1 - static_cast<float>(latIndex) / static_cast<float>(SUBDIVISION)};

            vertexData[startIndex].normal.x = vertexData[startIndex].position.x;
            vertexData[startIndex].normal.y = vertexData[startIndex].position.y;
            vertexData[startIndex].normal.z = vertexData[startIndex].position.z;

            vertexData[++startIndex].position = c;
            vertexData[startIndex].texCoord = {static_cast<float>(lonIndex + 1) / static_cast<float>(SUBDIVISION), 1 - static_cast<float>(latIndex) / static_cast<float>(SUBDIVISION)};

            vertexData[startIndex].normal.x = vertexData[startIndex].position.x;
            vertexData[startIndex].normal.y = vertexData[startIndex].position.y;
            vertexData[startIndex].normal.z = vertexData[startIndex].position.z;

	    	vertexData[++startIndex].position = b;
            vertexData[startIndex].texCoord = {static_cast<float>(lonIndex) / static_cast<float>(SUBDIVISION), 1 - static_cast<float>(latIndex + 1) / static_cast<float>(SUBDIVISION)};

            vertexData[startIndex].normal.x = vertexData[startIndex].position.x;
            vertexData[startIndex].normal.y = vertexData[startIndex].position.y;
            vertexData[startIndex].normal.z = vertexData[startIndex].position.z;

            vertexData[++startIndex].position = d;
            vertexData[startIndex].texCoord = {static_cast<float>(lonIndex + 1) / static_cast<float>(SUBDIVISION), 1 - static_cast<float>(latIndex + 1) / static_cast<float>(SUBDIVISION)};

            vertexData[startIndex].normal.x = vertexData[startIndex].position.x;
            vertexData[startIndex].normal.y = vertexData[startIndex].position.y;
            vertexData[startIndex].normal.z = vertexData[startIndex].position.z;
	    }
    }

    transform_ = {
        {1,1,1},
        {0,0,0},
        {0,0,0}
    };

    transformationMatrixResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), sizeof(TransformationMatrix));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrix_));
    transformationMatrix_->WVP = MathUtils::Matrix::MakeIdentity();
}

void Sphere::Update() {
    transform_.rotate.y += 0.003f;

    transformationMatrix_->World = MathUtils::Matrix::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 cameraMatrix = MathUtils::Matrix::MakeAffineMatrix(Camera.scale, Camera.rotate, Camera.translate);
	Matrix4x4 viewMatrix = cameraMatrix.Inverse();
    Matrix4x4 projectionMatrix = MathUtils::Matrix::MakePerspectiveFovMatrix(0.45f, float(CLIENT_WIDTH)/float(CLIENT_HEIGHT), 0.1f, 100);
    Matrix4x4 viewProjection = viewMatrix * projectionMatrix;
    transformationMatrix_->WVP = transformationMatrix_->World * viewProjection;

    EditParameterByImGui();
}

void Sphere::Draw() {
	Singleton<CommandController>::getInstance()->getList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
    Singleton<CommandController>::getInstance()->getList().Get()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    Singleton<CommandController>::getInstance()->getList().Get()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Singleton<CommandController>::getInstance()->getList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());
    Singleton<CommandController>::getInstance()->getList()->SetGraphicsRootConstantBufferView(3, Singleton<Light>::getInstance()->getDirectionalLight()->GetGPUVirtualAddress());
    if (textureHandle_){
    	Singleton<CommandController>::getInstance()->getList()->SetGraphicsRootDescriptorTable(2, textureHandle_.get_value());
    }
    Singleton<CommandController>::getInstance()->getList()->DrawInstanced(SUBDIVISION*SUBDIVISION*6, 1, 0, 0);
}

void Sphere::EditParameterByImGui() {
    #ifdef _DEBUG
    ImGui::Begin("Sphere");

    if (ImGui::TreeNode(uuid_.c_str())){
        ImGui::SliderFloat3("rotate", &transform_.rotate.x, -10, 10);
        ImGui::SliderFloat3("scale", &transform_.scale.x, 0, 3);
        ImGui::SliderFloat3("translate", &transform_.translate.x, -2, 2);
        ImGui::ColorEdit4("color", &material_->color.x);

        changeTexture(Singleton<TextureManager>::getInstance()->EditPropertyByImGui(textureName_));

        ImGui::TreePop();
    }
    ImGui::End();
    #endif
}
