#pragma once

#include <wrl/client.h>
#include "Singleton.h"

///Command control
class CommandController : Singleton<CommandController>{
    friend class Singleton<CommandController>;

	///Member
private:

	CommandController() {
	    System::Debug::Log(System::Debug::ConvertString(L"CommandClass Init\n"));
	}
	~CommandController() {
	    System::Debug::Log(System::Debug::ConvertString(L"CommandClass Delete\n"));
	}


    //Queue
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
    D3D12_COMMAND_QUEUE_DESC queueDesc_{};

    //allocator
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator_;

    //list
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list_;

public:
    //Generate
    void Generate();

    //Getter
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> getCommandQueue() const;

	D3D12_COMMAND_QUEUE_DESC getCommandQueueDesc() const;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> getAlloc() const;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> getList() const;
};
