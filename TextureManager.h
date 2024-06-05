#pragma once
#include <wrl/client.h>

#include "Singleton.h"
#include "DirectXTex/DirectXTex.h"

class TextureManager;

class Texture{
    friend Singleton<TextureManager>;
	DirectX::ScratchImage image_;
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
    D3D12_GPU_DESCRIPTOR_HANDLE shaderResourceViewHandle_{};

    //ID3D12DescriptorHeap* srvDescriptorHeap_;

public:
	//ユーザーは使うな。
	Texture(const std::string& name, ID3D12DescriptorHeap* srvDescriptorHeap);

    D3D12_GPU_DESCRIPTOR_HANDLE GetHandle() const {
        return shaderResourceViewHandle_;
    }

private:
	//MakeTexture 必要メソッド群
	DirectX::ScratchImage LoadTexture(const std::string& fileName);
	ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);
	D3D12_GPU_DESCRIPTOR_HANDLE MakeSRV(const DirectX::TexMetadata& metadata, ID3D12DescriptorHeap* srvDescriptorHeap, ID3D12Resource* textureResource) const;
};

class TextureManager : Singleton<TextureManager> {
    friend Singleton<TextureManager>;

    TextureManager() {
        System::Debug::Log(System::Debug::ConvertString(L"TextureManager Enabled\n"));
    }
	~TextureManager() {
        registeredTexturesMap_.clear();
        textureNameList_.clear();
        System::Debug::Log(System::Debug::ConvertString(L"TextureManager Disabled\n"));
    }

public:
    std::shared_ptr<Texture> Load(const std::string& name, ID3D12DescriptorHeap* srvDescriptorHeap);
    std::unordered_map <std::string, std::shared_ptr<Texture>> GetRegisteredTextures() const;

    std::vector<std::string> getNameList() const;

    std::string EditPropertyByImGui(const std::string& name) const;

    //StaticMethods
    static ID3D12Resource* CreateTextureResource(const DirectX::TexMetadata& metadata);
    static ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);
private:
    std::string resourceFolderPath = "Resources/";
	std::unordered_map<std::string, std::shared_ptr<Texture>> registeredTexturesMap_;
    std::vector<std::string> textureNameList_;
};
