#include "Heap.h"

#include "DeviceManager.h"

ID3D12DescriptorHeap* Heap::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT descriptorsNum, bool shaderVisible) {
    ID3D12DescriptorHeap* heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC desc {};
    desc.Type = type;
    desc.NumDescriptors = descriptorsNum;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

#ifdef _DEBUG
    HRESULT hr = 
#endif
       Singleton<DeviceManager>::getInstance()->getDevice().Get()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
    assert(SUCCEEDED(hr));
    
    return heap;
}

D3D12_CPU_DESCRIPTOR_HANDLE Heap::getCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE type,
	uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += (Singleton<DeviceManager>::getInstance()->getDevice().Get()->GetDescriptorHandleIncrementSize(type) * index);
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE Heap::getGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE type,
	uint32_t index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += (Singleton<DeviceManager>::getInstance()->getDevice().Get()->GetDescriptorHandleIncrementSize(type) * index);
    return handle;
}
