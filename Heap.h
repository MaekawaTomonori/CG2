#pragma once

namespace Heap{
	ID3D12DescriptorHeap* CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT descriptorsNum, bool shaderVisible);

    //制作途中につき非推奨 deprecation
    D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t index);
    //制作途中につき非推奨 deprecation
    D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t index);
};

