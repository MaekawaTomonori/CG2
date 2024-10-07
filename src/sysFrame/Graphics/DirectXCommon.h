#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

class DirectXCommon{
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;
public:
	void Initialize();

private:
	void CreateFactory();
	void CreateDevice();
};

