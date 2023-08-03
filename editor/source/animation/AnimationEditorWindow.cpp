#include "AnimationEditorWindow.h"
#include "../EditorContext.h"

#include <ugine/Profile.h>
#include <ugine/engine/gfx/ImGui.h>

namespace ugine::ed {

AnimationEditorWindow::AnimationEditorWindow(EditorContext& context)
    : EditorTool{ context, false } {
    auto& viewMenu{ context_.MainMenu().Get("View") };
    viewMenu.AddAction([this] { return ImGui::Checkbox("Animation preview", &visible_); });

    context_.Events().Connect<OpenResourceEvent<Animation>, &AnimationEditorWindow::OnOpenAnimation>(this);
}

void AnimationEditorWindow::SetAnimation(ResourceHandle<Animation> animation) {
    animation_ = animation;
    sequence_.SetAnimation(animation);
    visible_ = true;
}

void AnimationEditorWindow::Render() {
    PROFILE_EVENT_NC("AnimationEditorWindow", COLOR_EDITOR_PROFILE);

    if (ImGui::Begin("Animation editor", &visible_)) {
        ImSequencer::Sequencer(&sequence_, &currentFrame_, &expanded_, &selectedEntry_, &firstFrame_, ImSequencer::SEQUENCER_CHANGE_FRAME);
    }

    ImGui::End();
}

} // namespace ugine::ed