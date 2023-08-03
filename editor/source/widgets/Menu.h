#pragma once

#include <ugine/engine/gfx/ImGui.h>

#include <functional>
#include <vector>

namespace ImGuiEx {

struct MenuAction {
    using Callback = std::function<void()>;
    using Render = std::function<bool()>;

    std::string name;
    Callback callback;
    Render render;
};

class Menu {
public:
    class SubMenu {
    public:
        SubMenu(std::string_view name, bool root = false)
            : name_{ name }
            , isRoot_{ root } {}

        void AddAction(MenuAction action) { actions_.push_back(action); }
        void AddAction(std::string_view name, MenuAction::Callback callback) { actions_.push_back(MenuAction{ .name = name.data(), .callback = callback }); }
        void AddAction(MenuAction::Render render, MenuAction::Callback callback) { actions_.push_back(MenuAction{ .callback = callback, .render = render }); }
        void AddAction(MenuAction::Render render) { actions_.push_back(MenuAction{ .render = render }); }
        void AddSeparator() { actions_.push_back({}); }

        void Clear() {
            actions_.clear();
            submenus_.clear();
        }

        SubMenu& Get(std::string_view name);
        void Render();

    private:
        bool isRoot_{};
        std::string name_;
        std::vector<MenuAction> actions_;
        std::vector<std::unique_ptr<SubMenu>> submenus_;
    };

    Menu() = default;
    SubMenu& Get(std::string_view name);
    void Render();

private:
    SubMenu root_{ "root", true };
};

class ContextMenu {
public:
    void Open(std::vector<MenuAction>&& actions) {
        ImGui::OpenPopup("ContextMenu");
        actions_ = std::move(actions);
    }

    void Render();

private:
    std::vector<MenuAction> actions_;
};

} // namespace ImGuiEx