#pragma once

#include "../EditorTool.h"

namespace ugine::ed {

class StatsWidget : public EditorTool {
public:
    explicit StatsWidget(EditorContext& context);

    void Render() override;
};

} // namespace ugine::ed