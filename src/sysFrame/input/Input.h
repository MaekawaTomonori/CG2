#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <memory>
#include <wrl/client.h>

#include "Application/WinApp.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

class Input{
	Microsoft::WRL::ComPtr<IDirectInput8> directInput = nullptr;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard = nullptr;
	BYTE keyState[256] = {};
	BYTE preKey[256] = {};

	std::shared_ptr<WinApp> app_ = nullptr;

public:
	void Initialize(const std::shared_ptr<WinApp>& application);
	void Update();

	bool PushKey(BYTE key) const;
	bool TriggerKey(BYTE key) const;
	bool ReleaseKey(BYTE key) const;
};

