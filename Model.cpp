#include "Model.h"

#include <fstream>
#include <sstream>

ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& fileName) {
    ModelData modelData;
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
                modelData.vertices.push_back(vertex);
	        }
        }
    }
}
