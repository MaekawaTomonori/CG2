#pragma once
#include "MathUtils.h"
#include "Object.h"

class Sphere : public Object{
public:
	void Initialize() override;
	void Update() override;
	void Draw() override;

private:
	void EditParameterByImGui() override;

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;

	const uint32_t SUBDIVISION = 16;
    const float LAT_EVERY = MathUtils::F_PI / SUBDIVISION;
    const float LON_EVERY = (2 * MathUtils::F_PI) / SUBDIVISION;
};

