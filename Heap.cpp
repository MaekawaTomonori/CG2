#include "Heap.h"

ID3D12DescriptorHeap* Heap::CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT descriptorsNum, bool shaderVisible) {
    ID3D12DescriptorHeap* heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC desc {};
    desc.Type = type;
    desc.NumDescriptors = descriptorsNum;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
    assert(SUCCEEDED(hr));
    
    return heap;
}
