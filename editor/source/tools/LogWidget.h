#pragma once

#include <ugine/Span.h>
#include <ugine/String.h>
#include <ugine/Vector.h>

#include <ugine/engine/gfx/ImGui.h>

#include "../EditorEvents.h"
#include "../EditorTool.h"

namespace ugine::ed {

class EditorContext;

class LogWidget : public EditorTool {
public:
    explicit LogWidget(EditorContext& context);

    void Add(ImVec4 color, u64 millis, Span<const char> text);
    void Clear();

    void Render() override;

private:
    struct Entry {
        ImVec4 color;
        String text;
        char timestamp[24];
    };

    void RenderContent();
    void OnLogEvent(const LogEvent& event);

    std::array<Entry, 128> lines_;
    u32 begin_{};
    u32 size_{};

    AtomicSpinLock mutex_;
    bool autoScroll_{ true };
    bool scrollToBottom_{};
};

} // namespace ugine::ed