#pragma once

#include "../EditorEvents.h"
#include "../EditorTool.h"

#include <ugine/engine/gfx/Animation.h>
#include <ugine/engine/gfx/Model.h>

#include <ImSequencer.h>

namespace ugine::ed {

class EditorContext;

class AnimationEditorWindow : public EditorTool {
public:
    explicit AnimationEditorWindow(EditorContext& context);

    void SetAnimation(ResourceHandle<Animation> animation);
    void Render() override;

private:
    class Sequence : public ImSequencer::SequenceInterface {
    public:
        Sequence() = default;

        void SetAnimation(ResourceHandle<Animation> animation) {
            bones_.clear();
            lengthInSeconds_ = animation->lengthSeconds;
            for (auto [key, _] : animation->channels) {
                bones_.push_back(key);
            }
            items_.push_back(Item{ 0, 2, 3 });
            items_.push_back(Item{ 1, 5, 6 });
        }

        int GetFrameMin() const override { return 0; }
        int GetFrameMax() const override { return 10; }
        int GetItemCount() const override { return int(items_.size()); }

        //virtual void BeginEdit(int /*index*/) {}
        //virtual void EndEdit() {}

        int GetItemTypeCount() const override { return 3; }
        const char* GetItemTypeName(int typeIndex) const override {
            switch (typeIndex) {
            case 0: return "Position";
            case 1: return "Rotation";
            case 2: return "Scale";
            default: return "n/a";
            }
        }
        const char* GetItemLabel(int index) const override {
            switch (index) {
            case 0: return "item1";
            default: return "item2";
            }
        }

        //const char* GetCollapseFmt() const override { return "%d Frames / %d entries"; }

        void Get(int index, int** start, int** end, int* type, unsigned int* color) override {
            auto& item{ items_[index] };

            if (start) {
                *start = &item.frameStart;
            }

            if (end) {
                *end = &item.frameEnd;
            }

            if (type) {
                *type = item.type;
            }

            if (color) {
                *color = index == 0 ? 0xff0000ff : 0xffff00ff;
            }
        }

        //void Add(int /*type*/) override {}
        //void Del(int /*index*/) override {}
        //void Duplicate(int /*index*/) override {}

        //void Copy() {}
        //void Paste() {}

        //virtual size_t GetCustomHeight(int /*index*/) { return 0; }
        //virtual void DoubleClick(int /*index*/) {}
        //virtual void CustomDraw(int /*index*/, ImDrawList* /*draw_list*/, const ImRect& /*rc*/, const ImRect& /*legendRect*/, const ImRect& /*clippingRect*/,
        //    const ImRect& /*legendClippingRect*/) {}
        //virtual void CustomDrawCompact(int /*index*/, ImDrawList* /*draw_list*/, const ImRect& /*rc*/, const ImRect& /*clippingRect*/) {}

    private:
        struct Item {
            int type{};
            int frameStart{};
            int frameEnd{};
        };

        f32 lengthInSeconds_{};
        std::vector<StringID> bones_;
        std::vector<Item> items_;
    };

    void OnOpenAnimation(OpenResourceEvent<Animation> event) { SetAnimation(event.resource); }

    ResourceHandle<Animation> animation_;

    Sequence sequence_;
    int currentFrame_{};
    bool expanded_{ true };
    int selectedEntry_{};
    int firstFrame_{};
};

} // namespace ugine::ed