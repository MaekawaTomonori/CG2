#pragma once
struct ModelData{
	std::vector<VertexData> vertices;
};

class Model{
public:
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& fileName);
};

