#pragma once
#include <queue>

//全体管理
class ImGuiManager : Singleton<ImGuiManager>{
    friend Singleton<ImGuiManager>;

    ImGuiManager() {
        System::Debug::Log(System::Debug::ConvertString(L"ImGuiManager Enabled\n"));
    }
	~ImGuiManager() {
        System::Debug::Log(System::Debug::ConvertString(L"ImGuiManager Disabled\n"));
    }

    //std::queue<std::function<void()>> drawQueue_;

public:
	//void Initialize();
	//void Finalize();

	void Begin() const;
    void Draw() const;
    void End() const;

	//void AddQueue(const std::function<void()>& func);
};

