#include "Input.h"

#include <cassert>
#include <wrl/client.h>

void Input::Initialize(const std::shared_ptr<Application>& application) {
    app_ = application;
    HRESULT hr = DirectInput8Create(app_->GetWindowClass().hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
    assert(SUCCEEDED(hr));

    hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, nullptr);
    assert(SUCCEEDED(hr));

    hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));

    hr = keyboard->SetCooperativeLevel(app_->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    assert(SUCCEEDED(hr));
}

void Input::Update() {
    memcpy(preKey, keyState, sizeof(keyState));

    HRESULT hr;

	hr = keyboard->Acquire();

    assert(SUCCEEDED(hr));

    hr = keyboard->GetDeviceState(sizeof(keyState), keyState);
    assert(SUCCEEDED(hr));
}

bool Input::PushKey(BYTE key) const {
    return keyState[key];
}

bool Input::TriggerKey(BYTE key) const {
    return keyState[key] && !preKey[key];
}

bool Input::ReleaseKey(BYTE key) const {
    return !keyState[key] && preKey[key];
}
