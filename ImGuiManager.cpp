#include "Pch.h"
#include "ImGuiManager.h"

#include "CommandController.h"

void ImGuiManager::Begin() const {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::Draw() const {
    /*while (!drawQueue_.empty()){
        auto& func = drawQueue_.front();
        func();
	}*/

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Singleton<CommandController>::getInstance()->getList().Get());
}

void ImGuiManager::End() const {
    ImGui::Render();
}

void ImGuiManager::AddQueue(const std::function<void()>& func) {
    drawQueue_.push(func);
}
