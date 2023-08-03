#pragma once

#include "../EditorTool.h"

namespace ugine::ed {

class CameraPreview : public EditorTool {
public:
    explicit CameraPreview(EditorContext& context);

    void Render() override;
};

} // namespace ugine::ed