#include "TextureManager.h"

#include "CommandController.h"
#include "DeviceManager.h"
#include "Heap.h"
#include "Shader.h"
#include "DirectXTex/d3dx12.h"

Texture::Texture(const std::string& name, ID3D12DescriptorHeap* srvDescriptorHeap) {
    image_ = LoadTexture(name);
    const DirectX::TexMetadata& metaData = image_.GetMetadata();
    textureResource_ = Singleton<TextureManager>::getInstance()->CreateTextureResource(metaData);
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = UploadTextureData(textureResource_.Get(), image_);

    shaderResourceViewHandle_ = MakeSRV(metaData, srvDescriptorHeap, textureResource_.Get());
}

std::shared_ptr<Texture> TextureManager::Load(const std::string& name, ID3D12DescriptorHeap* srvDescriptorHeap) {
    //テクスチャが新規ロードされた場合、Mapに登録し、名前をリストに登録しておく。
    //hashmapのkeyを全取得する関数を見つけられたら名前リストは解雇。
    if (registeredTexturesMap_.try_emplace(name, new Texture(resourceFolderPath + name, srvDescriptorHeap)).second){
        textureNameList_.push_back(name);
    }
    return registeredTexturesMap_.at(name);
}

std::unordered_map<std::string, std::shared_ptr<Texture>> TextureManager::GetRegisteredTextures() const {
    return registeredTexturesMap_;
}

std::vector<std::string> TextureManager::getNameList() const {
    return textureNameList_;
}

std::string TextureManager::EditPropertyByImGui(const std::string& name) const {
    const char* currentItem = name.c_str();

    if(ImGui::BeginCombo("Texture", currentItem)){
        for (int n = 0; n < textureNameList_.size(); ++n){
            bool isSelected = (currentItem == textureNameList_[n]);
            if(ImGui::Selectable(textureNameList_[n].c_str()), isSelected){
                currentItem = textureNameList_[n].c_str();
            }
            if(isSelected){
                ImGui::SetItemDefaultFocus();
            }
	    }
        ImGui::EndCombo();
    }

    return currentItem;
}

DirectX::ScratchImage Texture::LoadTexture(const std::string& fileName) {
    DirectX::ScratchImage image {};
    std::wstring filePathW = System::Debug::ConvertString(fileName);

    HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
    assert(SUCCEEDED(hr));

    //Make Minimap
    DirectX::ScratchImage mipImages {};
    hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
    assert(SUCCEEDED(hr));

    return mipImages;
}

ID3D12Resource* TextureManager::CreateTextureResource(const DirectX::TexMetadata& metadata) {
    /// FLOW  ///
    /// 1. Resource setting from metadata
    /// 2. Heap setting
    /// 3. Generate Resource
    ///

    //Step1
    //Setting Resource from Metadata
    D3D12_RESOURCE_DESC resourceDesc {};
    resourceDesc.Width = UINT(metadata.width);
    resourceDesc.Height = UINT(metadata.height);
    resourceDesc.MipLevels = UINT16(metadata.mipLevels);
    resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
    resourceDesc.Format = metadata.format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

    //Step2
    //HEAP SETTINGs
    D3D12_HEAP_PROPERTIES heapProperties {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    //heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    //heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

    //Step3
    //Generate Resource
    ID3D12Resource* resource = nullptr;

#ifdef _DEBUG
	HRESULT hr = 
#endif  
    Singleton<DeviceManager>::getInstance()->getDevice().Get()->CreateCommittedResource(
        &heapProperties, 
        D3D12_HEAP_FLAG_NONE, 
        &resourceDesc, 
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, 
        IID_PPV_ARGS(&resource)
    );
    assert(SUCCEEDED(hr));

    return resource;
}

//Attribute 返り値無視をユルサナイ属性。メンヘラメソッド。
[[nodiscard]]
ID3D12Resource* Texture::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {
    std::vector<D3D12_SUBRESOURCE_DATA> subResources;
    DirectX::PrepareUpload(Singleton<DeviceManager>::getInstance()->getDevice().Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subResources);
    uint32_t intermediateSize = (uint32_t)GetRequiredIntermediateSize(texture, 0, UINT(subResources.size()));
    ID3D12Resource* intermediateResourse = Shader::CreateBufferResource(Singleton<DeviceManager>::getInstance()->getDevice().Get(), intermediateSize);
    UpdateSubresources(Singleton<CommandController>::getInstance()->getList().Get(), texture, intermediateResourse, 0, 0, UINT(subResources.size()), subResources.data());

    D3D12_RESOURCE_BARRIER barrier {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    Singleton<CommandController>::getInstance()->getList().Get()->ResourceBarrier(1, &barrier);
    return intermediateResourse;
}

D3D12_GPU_DESCRIPTOR_HANDLE Texture::MakeSRV(const DirectX::TexMetadata& metadata, ID3D12DescriptorHeap* srvDescriptorHeap, ID3D12Resource* textureResource) const {
    //Setting SRV from metadata
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc {};
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

    //decide where to place the descriptor heaps
    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = Heap::getCPUDescriptorHandle(srvDescriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, (uint32_t)Singleton<TextureManager>::getInstance()->GetRegisteredTextures().size() + 1);
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = Heap::getGPUDescriptorHandle(srvDescriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, (uint32_t)Singleton<TextureManager>::getInstance()->GetRegisteredTextures().size() + 1);
    Singleton<DeviceManager>::getInstance()->getDevice().Get()->CreateShaderResourceView(textureResource, &srvDesc, textureSrvHandleCPU);
    return textureSrvHandleGPU;
}

ID3D12Resource* TextureManager::CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {
    D3D12_RESOURCE_DESC resourceDesc {};
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.MipLevels = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProperties {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE depthClearValue {};
    depthClearValue.DepthStencil.Depth = 1.f;
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ID3D12Resource* resource = nullptr;
#ifdef _DEBUG
    HRESULT hr = 
#endif 
        device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&resource)
    );
    assert(SUCCEEDED(hr));

    return resource;
}
