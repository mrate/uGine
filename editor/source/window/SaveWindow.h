#pragma once

#include "Window.h"

#include <ugine/FileSystem.h>

#include <functional>

namespace ugine::ed {

class EditorContext;

class SaveWindow : public ModalButtonWindow {
public:
    using SaveAction = std::function<void(const Path& /*targetPath*/, StringView /*fileName*/)>;

    explicit SaveWindow(EditorContext& context)
        : ModalButtonWindow{ context }
        , context_{ context } {}

    void SetSaveAction(StringView fileName, SaveAction action) {
        fileName = fileName;
        saveAction_ = action;
    }

    const char* Name() const override { return "Save world"; }
    void Init() override;
    void Show() override;

private:
    EditorContext& context_;

    SaveAction saveAction_;
    Path targetPath_{ FileSystem::CurrentPath() };
    String fileName_{};
};

} // namespace ugine::ed