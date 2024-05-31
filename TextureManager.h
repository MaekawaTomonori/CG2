#pragma once
#include <wrl/client.h>

#include "Singleton.h"
#include "DirectXTex/DirectXTex.h"

class Texture{
    DirectX::ScratchImage image_;
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
    D3D12_GPU_DESCRIPTOR_HANDLE shaderResourceViewHandle_{};

public:
	//Texture();
    Texture(const std::string& name, ID3D12DescriptorHeap* srvDescHeap);

    D3D12_GPU_DESCRIPTOR_HANDLE getHandle() const {
        return shaderResourceViewHandle_;
    }
};

class TextureManager : Singleton<TextureManager> {
    friend Singleton<TextureManager>;
    public:
    DirectX::ScratchImage LoadTexture(const std::string& fileName);
    void UploadTexture(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);
    D3D12_GPU_DESCRIPTOR_HANDLE MakeSRV(const DirectX::TexMetadata& metadata, ID3D12DescriptorHeap* srvDescriptorHeap, ID3D12Device* device, ID3D12Resource* textureResource);

    //StaticMethods
    static ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);
    static ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);
private:
    //std::string resourceFolderPath;
	//std::unordered_map<std::string&, Texture*> textureMap;
};

