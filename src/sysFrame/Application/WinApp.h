#pragma once
#include <inttypes.h>
#include <windows.h>

class WinApp{
	HWND hwnd_ = nullptr;
	WNDCLASS wc_ {};
public:
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;
private:
	bool Create();
public:
	void Initialize();
	void Update();
	void Finalize() const;

	bool ProcessMessage();

public: //Getter
	HWND GetHwnd() const;
    WNDCLASS GetWindowClass() const;
};

