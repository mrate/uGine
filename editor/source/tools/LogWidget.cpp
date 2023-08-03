#include "LogWidget.h"

#include "../EditorContext.h"
#include "../widgets/ImGuiScope.h"

namespace ugine::ed {

LogWidget::LogWidget(EditorContext& context)
    : EditorTool{ context } {
    auto& view{ context_.MainMenu().Get("View") };
    view.AddAction([this] { return ImGui::Checkbox("Log", &visible_); });

    context_.Events().Connect<LogEvent, &LogWidget::OnLogEvent>(this);
}

void LogWidget::Add(ImVec4 color, u64 millis, Span<const char> text) {
    Entry entry{
        .color = color,
        .text = text.Data(),
    };

    const auto seconds{ int(millis / 1000) };
    const auto minutes{ int(seconds / 60) };
    const auto hours{ int(minutes / 60) };

    sprintf(entry.timestamp, "[%d:%02d:%02d.%03d]", hours, minutes % 60, seconds % 60, int(millis % 1000));

    Lock lock{ mutex_ };

    int position{};
    if (size_ < lines_.size()) {
        position = begin_ + size_;
        ++size_;
    } else {
        position = begin_;
        begin_ = (begin_ + 1) % lines_.size();
    }

    lines_[position] = entry;

    if (autoScroll_) {
        scrollToBottom_ = true;
    }
}

void LogWidget::Clear() {
    size_ = 0;
}

void LogWidget::Render() {
    if (ImGui::Begin(ICON_FA_TEXT_BOX " Log")) {
        if (ImGui::BeginChildFrame(1, ImGui::GetContentRegionAvail())) {
            RenderContent();

            ImGui::EndChildFrame();
        }
    }
    ImGui::End();
}

void LogWidget::RenderContent() {
    {
        Lock lock{ mutex_ };

        auto drawList{ ImGui::GetWindowDrawList() };

        for (u32 i{}; i < size_; ++i) {
            const auto& line{ lines_[(begin_ + i) % lines_.size()] };
            //ImScope::Color color{ ImGuiCol_Text, line.color };

            const auto posMin{ ImGui::GetCursorScreenPos() };
            const ImVec2 posMax{ posMin.x + 5, posMin.y + ImGui::GetFontSize() };

            drawList->AddRectFilled(posMin, posMax, IM_COL32(255 * line.color.x, 255 * line.color.y, 255 * line.color.z, 255 * line.color.w));
            ImGui::Dummy(ImVec2{ 5, ImGui::GetFontSize() });
            ImGui::SameLine();

            ImGui::Text("%s %s", line.timestamp, line.text.Data());
        }
    }

    if (scrollToBottom_) {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom_ = false;
    }
}

void LogWidget::OnLogEvent(const LogEvent& event) {
    std::array<ImVec4, 5> Colors = {
        ImVec4{ 0.7f, 0.7f, 0.7f, 1 },
        ImVec4{ 0.5f, 0.5f, 0.5f, 1 },
        ImVec4{ 0, 0.75f, 0, 1 },
        ImVec4{ 0.75f, 0.25f, 0.1f, 1 },
        ImVec4{ 0.7f, 0.1f, 0.1f, 1 },
    };

    Add(Colors[int(event.level)], event.millis, event.msg);
}

namespace {
    EditorToolset::Register _{ [](EditorContext& context) { context.RegisterEditorTool(MakeUnique<LogWidget>(context.Allocator(), context)); } };
} // namespace

} // namespace ugine::ed