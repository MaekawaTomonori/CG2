#include "CommandController.h"

#include "DeviceManager.h"


Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandController::getCommandQueue() const {
	return queue_;
}

D3D12_COMMAND_QUEUE_DESC CommandController::getCommandQueueDesc() const {
	return queueDesc_;
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandController::getAlloc() const {
	return allocator_;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandController::getList() const {
	return list_;
}

void CommandController::Generate() {
#define device Singleton<DeviceManager>::getInstance()
	HRESULT hResult = device->getDevice()->CreateCommandQueue(&queueDesc_, IID_PPV_ARGS(&queue_));

	assert(SUCCEEDED(hResult));
	hResult = device->getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator_));
    assert(SUCCEEDED(hResult));

	hResult = device->getDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator_.Get(), nullptr, IID_PPV_ARGS(&list_));
	assert(SUCCEEDED(hResult));
#undef device
}

