#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <memory>
#include <wrl/client.h>

#include "Application/Application.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

class Input{
	Microsoft::WRL::ComPtr<IDirectInput8> directInput = nullptr;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard = nullptr;
	BYTE keyState[256] = {};
	BYTE preKey[256] = {};

	std::shared_ptr<Application> app_ = nullptr;

public:
	void Initialize(const std::shared_ptr<Application>& application);
	void Update();

	bool PushKey(BYTE key) const;
	bool TriggerKey(BYTE key) const;
	bool ReleaseKey(BYTE key) const;
};

