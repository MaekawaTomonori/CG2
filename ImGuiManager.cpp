#include "Pch.h"
#include "ImGuiManager.h"

#include "CommandController.h"

void ImGuiManager::Begin() const {
#ifdef _DEBUG
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
#endif
}

void ImGuiManager::Draw() const {
#ifdef _DEBUG
    /*while (!drawQueue_.empty()){
        auto& func = drawQueue_.front();
        func();
	}*/

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Singleton<CommandController>::getInstance()->getList().Get());
#endif
}

void ImGuiManager::End() const {
#ifdef _DEBUG
    ImGui::Render();
#endif
}

//void ImGuiManager::AddQueue(const std::function<void()>& func) {
//    drawQueue_.push(func);
//}
