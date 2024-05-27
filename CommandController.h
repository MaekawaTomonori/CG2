#pragma once

#include <wrl/client.h>
#include "Singleton.h"

///Command control
class CommandController : Singleton<CommandController>{
    friend class Singleton<CommandController>;

	///Member
private:
    //Queue
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_ = nullptr;
    D3D12_COMMAND_QUEUE_DESC queueDesc_{};

    //allocator
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator_ = nullptr;

    //list
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list_ = nullptr;

public:
    //Generate
    void Generate();

    //Getter
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> getCommandQueue() const;

	D3D12_COMMAND_QUEUE_DESC getCommandQueueDesc() const;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> getAlloc() const;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> getList() const;
};
