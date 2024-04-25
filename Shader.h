#pragma once
class Shader{
public:
    static ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);
};

