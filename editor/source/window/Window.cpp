#include "Window.h"
#include "../EditorContext.h"

#include "../widgets/ImGuiEx.h"

namespace ugine::ed {

void ButtonWindow::BeginContent() {
    size_ = ImGui::GetContentRegionAvail();
    ImGui::BeginChildFrame(1, ImVec2{ size_.x, size_.y - ButtonsHeight() - 2 * ImGui::GetStyle().ItemSpacing.y });
}

void ButtonWindow::EndContent() {
    ImGui::EndChildFrame();
}

void ButtonWindow::Buttons() {
    auto buttons = [](const auto& buttons) {
        auto pressed{ uint32_t(buttons.Size()) };
        for (uint32_t index{}; const auto& [enabled, name, _] : buttons) {
            ImScope::Disabled disabled{ !enabled };

            if (ImGui::Button(name.Data())) {
                pressed = index;
            }

            ImGui::SameLine();

            ++index;
        }
        return pressed;
    };

    ImGui::BeginChildFrame(2, ImVec2{ size_.x, ButtonsHeight() });

    const auto position{ ImGui::GetCursorPos() };
    const auto leftPressed{ buttons(leftButtons_) };

    ImGui::SetCursorPos(ImVec2{ position.x + size_.x - RightButtonsWidth() - ImGui::GetStyle().WindowPadding.x, position.y });

    const auto rightPressed{ buttons(rightButtons_) };

    ImGui::EndChildFrame();

    if (leftPressed < leftButtons_.Size()) {
        leftButtons_[leftPressed].func();
    }

    if (rightPressed < rightButtons_.Size()) {
        rightButtons_[rightPressed].func();
    }
}

void ButtonWindow::SetLeftButton(const Button& button) {
    leftButtons_.Clear();
    leftButtons_.PushBack(button);
}

void ButtonWindow::SetLeftButtons(Vector<Button> buttons) {
    leftButtons_ = std::move(buttons);
}

void ButtonWindow::SetRightButton(const Button& button) {
    rightButtons_.Clear();
    rightButtons_.PushBack(button);

    CalcRightSize();
}

void ButtonWindow::SetRightButtons(Vector<Button> buttons) {
    rightButtons_ = std::move(buttons);
    CalcRightSize();
}

void ButtonWindow::CalcRightSize() {
    const auto spacing{ ImGui::GetStyle().ItemSpacing.x };
    const auto padding{ ImGui::GetStyle().FramePadding.x };

    float width{};
    for (const auto& button : rightButtons_) {
        width += spacing + ImGui::CalcTextSize(button.name.Data()).x + 2 * padding;
    }

    if (!rightButtons_.Empty()) {
        width -= spacing;
    }

    rightButtonsSize_ = width;
}

void ButtonWindow::SetRightButtonEnabled(int i, bool enabled) {
    if (i < rightButtons_.Size()) {
        rightButtons_[i].enabled = enabled;
    } else {
        UGINE_ERROR("Invalid right button: {} of {}", i, rightButtons_.Size());
    }
}

float ButtonWindow::RightButtonsWidth() const {
    return rightButtonsSize_;
}

float ButtonWindow::ButtonsHeight() const {
    const auto padding{ ImGui::GetStyle().FramePadding.y };
    return 2 * padding + 2 * padding + ImGui::GetFontSize();
}

ModalButtonWindow::ModalButtonWindow(EditorContext& context) {
    SetLeftButton(Button{
        .name = ICON_FA_WINDOW_CLOSE " Cancel",
        .func = [this, &context] { context.Events().CloseModal(this); },
    });
}

void MessageWindow::Show() {
    static const char* symbol[] = {
        ICON_FA_ALERT_OCTAGON,
        ICON_FA_ALERT,
        ICON_FA_INFORMATION,
    };

    ImGui::Text("%s %s", symbol[int(type_)], text_.Data());

    if (ImGui::Button("OK")) {
        context_.Events().CloseModal(this);
    }
}

} // namespace ugine::ed