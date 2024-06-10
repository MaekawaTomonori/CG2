#pragma once
#include "Object.h"

struct ModelData{
	std::vector<VertexData> vertices;
};

class Model : public Object {
private:
ModelData modelData_;
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;

	void LoadObjFile(const std::string& directoryPath, const std::string& fileName);
	//現時点のこのクラスでは引数が存在する初期化関数のみを使用するため、引数なしの関数を非公開に設定
	void Initialize() override;

public:
	void Initialize(const std::string& directoryPath, const std::string& fileName);
	void Update() override;
	void Draw() override;

private:
	void EditParameterByImGui() override;
};

