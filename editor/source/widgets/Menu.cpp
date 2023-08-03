#include "Menu.h"

namespace ImGuiEx {

Menu::SubMenu& Menu::SubMenu::Get(std::string_view name) {
    for (auto& submenu : submenus_) {
        if (submenu->name_ == name) {
            return *submenu;
        }
    }

    submenus_.push_back(std::make_unique<SubMenu>(name));
    return *submenus_.back();
}

void Menu::Render() {
    root_.Render();
}

void Menu::SubMenu::Render() {
    if (!isRoot_) {
        if (!ImGui::BeginMenu(name_.c_str())) {
            return;
        }
    }

    for (auto& submenu : submenus_) {
        submenu->Render();
    }

    for (auto& action : actions_) {
        if (action.name.empty() && !action.render) {
            ImGui::Separator();
            continue;
        }

        bool invoked{};
        if (action.render) {
            invoked = action.render();
        } else {
            invoked = ImGui::Selectable(action.name.c_str());
        }
        if (invoked && action.callback) {
            action.callback();
        }
    }

    if (!isRoot_) {
        ImGui::EndMenu();
    }
}

Menu::SubMenu& Menu::Get(std::string_view name) {
    return root_.Get(name);
}

void ContextMenu::Render() {
    if (ImGui::BeginPopup("ContextMenu")) {
        for (const auto& action : actions_) {
            if (action.name.empty() && !action.render) {
                ImGui::Separator();
                continue;
            }

            if (action.render) {
                action.render();
            } else {
                if (ImGui::Selectable(action.name.c_str())) {
                    action.callback();
                }
            }
        }
        ImGui::EndPopup();
    }
}

} // namespace ImGuiEx