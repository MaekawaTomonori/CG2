#include "Model.h"

#include <fstream>
#include <sstream>

#include "CommandController.h"
#include "DeviceManager.h"
#include "Light.h"
#include "MathUtils.h"
#include "Shader.h"

void Model::LoadObjFile(const std::string& directoryPath, const std::string& fileName) {
    std::vector<Vector4> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::string line;

    std::ifstream file(directoryPath + "/" + fileName);
    assert(file.is_open());

    while(std::getline(file, line)){
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if(identifier == "v"){
            Vector4 position;
            s >> position.x >> position.y >> position.z;
            position.w = 1.f;
            positions.push_back(position);
        }else if(identifier == "vt"){
            Vector2 texcoord;
            s >> texcoord.x >> texcoord.y;
            texcoords.push_back(texcoord);
        }else if(identifier == "vn"){
            Vector3 normal;
            s >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }else if(identifier == "f"){
            for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex){
                std::string vertexDefinition;
                s >> vertexDefinition;
            	std::istringstream v(vertexDefinition);

            	uint32_t elementIndices[3];
                for (int32_t element = 0; element < 3; ++element){
                    std::string index;
                    std::getline(v, index, '/');
                    elementIndices[element] = std::stoi(index);
                }

            	Vector4 position = positions[elementIndices[0] - 1];
                Vector2 texcoord = texcoords[elementIndices[1] - 1];
                Vector3 normal = normals[elementIndices[2] - 1];

                VertexData vertex = {position, texcoord, normal};
            	modelData_.vertices.push_back(vertex);
	        }
        }
    }
}

void Model::Initialize() {
    transform_ = {
        {1,1,1},
        {0,0,0},
        {0,0,0}
    };

    transformationMatrixResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice(), sizeof(TransformationMatrix));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrix_));
    transformationMatrix_->WVP = MathUtils::Matrix::MakeIdentity();

    materialResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice(), sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&material_));
    material_->color = {1,1,1,1};
    material_->enableLighting = true;
    material_->uvTransform = MathUtils::Matrix::MakeIdentity();
}

void Model::Initialize(const std::string& directoryPath, const std::string& fileName) {
    LoadObjFile(directoryPath, fileName);

    vertexResource_ = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice(), sizeof(VertexData) * modelData_.vertices.size());

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    std::memcpy(vertexData, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());

    //Initialize();
}

void Model::Update() {
    transformationMatrix_->World = MathUtils::Matrix::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    transformationMatrix_->WVP = transformationMatrix_->World *
	    MathUtils::Matrix::MakeAffineMatrix(Camera.scale, Camera.rotate, Camera.translate).Inverse() *
	    MathUtils::Matrix::MakePerspectiveFovMatrix(
		    0.45f, static_cast<float>(CLIENT_HEIGHT) / static_cast<float>(CLIENT_WIDTH), 0.1f, 100.f);
    EditParameterByImGui();
}

void Model::Draw() {
    Singleton<CommandController>::getInstance()->getList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	Singleton<CommandController>::getInstance()->getList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //Singleton<CommandController>::getInstance()->getList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    //Singleton<CommandController>::getInstance()->getList()->SetComputeRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());
    //Singleton<CommandController>::getInstance()->getList()->SetComputeRootConstantBufferView(3, Singleton<Light>::getInstance()->getDirectionalLight()->GetGPUVirtualAddress());
    Singleton <CommandController>::getInstance()->getList()->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
}

void Model::EditParameterByImGui() {
}
