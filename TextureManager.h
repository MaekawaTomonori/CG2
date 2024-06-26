﻿#pragma once
#include "DirectXTex/DirectXTex.h"

class TextureManager{
    public:
    static DirectX::ScratchImage LoadTexture(const std::string& fileName);
    static ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);
    static ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);
    static void UploadTexture(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);
    static D3D12_GPU_DESCRIPTOR_HANDLE MakeSRV(const DirectX::TexMetadata& metadata, ID3D12DescriptorHeap* srvDescriptorHeap, ID3D12Device* device, ID3D12Resource* textureResource);
private:
    //std::string resourceFolderPath;
    //std::unordered_map<std::string&, DirectX::ScratchImage> textureMap;
};

