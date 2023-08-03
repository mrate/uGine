#pragma once

#include <functional>
#include <vector>

namespace ugine::ed {

class EditorContext;

class EditorToolset {
public:
    using EditorToolRegistrator = std::function<void(EditorContext&)>;

    struct Register {
        Register(EditorToolRegistrator registrator) { EditorToolset::RegisterEditorToolset(registrator); }
    };

    static void RegisterEditorToolset(EditorToolRegistrator registrator) { registrators_.push_back(registrator); }

    static void RegisterEditor(EditorContext& context) {
        for (auto& registrator : registrators_) {
            registrator(context);
        }
    }

private:
    inline static std::vector<EditorToolRegistrator> registrators_;
};

class EditorTool {
public:
    explicit EditorTool(EditorContext& context, bool visible = true)
        : context_{ context }
        , visible_{ visible } {}

    virtual ~EditorTool() = default;

    bool Visible() const { return visible_; }

    virtual void Render() = 0;

    EditorContext& Context() { return context_; }

protected:
    EditorContext& context_;
    bool visible_{ true };
};

} // namespace ugine::ed