#pragma once

#include "../EditorTool.h"

namespace ugine::ed {

class EditorContext;

class CVarWindow : public EditorTool {
public:
    explicit CVarWindow(EditorContext& context);

    void Render() override;
};

} // namespace ugine::ed