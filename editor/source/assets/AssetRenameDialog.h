#pragma once

#include "../window/Window.h"

#include <ugine/Path.h>
#include <ugine/String.h>

#include <ugine/engine/core/Resource.h>

namespace ugine::ed {

class AssetRenameDialog : public ModalButtonWindow {
public:
    explicit AssetRenameDialog(EditorContext& context)
        : ModalButtonWindow{ context }
        , context_{ context } {}

    const char* Name() const override { return "Rename"; }
    void Init() override;
    void Show() override;

    void Rename(const ResourceID& id);

private:
    EditorContext& context_;
    StaticString<256> newName_;
    ResourceID id_;
    Path newPath_;
};

} // namespace ugine::ed