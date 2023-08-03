#include "StatsWidget.h"

#include "../EditorContext.h"
#include "../widgets/PropertyTable.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/GraphicsScene.h>
#include <ugine/engine/gfx/GraphicsState.h>

namespace ugine::ed {

const char* StatsVisibleSettings{ "stats-visible" };

StatsWidget::StatsWidget(EditorContext& context)
    : EditorTool{ context, context.Settings().Get<bool>(StatsVisibleSettings, false) } {
    auto& view{ context.MainMenu().Get("View") };
    view.AddAction([this] { return ImGui::Checkbox("Stats", &visible_); }, [this] { context_.Settings().Set<bool>(StatsVisibleSettings, visible_); });
}

void StatsWidget::Render() {
    if (ImGui::Begin(ICON_FA_RULER " Stats", &visible_)) {
        auto state{ context_.Engine().GetState<GraphicsState>() };

        const auto flags{ ImGuiTreeNodeFlags_DefaultOpen };

        auto drawMs = [](PropertyTable& table, const char* name, auto v, auto t) {
            table.BeginProperty(name);
            ImGui::Text("%0.4f ms", v);
            ImGui::SameLine();
            ImGui::ProgressBar(v / t);
            table.EndProperty();
        };

        if (ImGui::CollapsingHeader(ICON_FA_WATCH " Time", flags)) {
            const auto& stats{ context_.Engine().GetFrameStats() };

            PropertyTable table{ "Time", &context_ };

            drawMs(table, "Early update", stats.earlyUpdateMS, stats.frameTimeMS);
            drawMs(table, "Update", stats.updateMS, stats.frameTimeMS);
            drawMs(table, "Late update", stats.lateUpdateMS, stats.frameTimeMS);
            drawMs(table, "Sync", stats.syncMS, stats.frameTimeMS);
            drawMs(table, "Filesystem", stats.filesystemMS, stats.frameTimeMS);

            table.ConstPropertyUnformatted("Frame", std::format("{:0.4f} ms", stats.frameTimeMS).c_str());
        }

        if (ImGui::CollapsingHeader(ICON_FA_MEMORY " Memory", flags)) {
            PropertyTable table{ "Memory", &context_ };

            table.ConstPropertyUnformatted("Frame allocations", std::format("{}", context_.Engine().GetFrameAllocations()).c_str());
        }

        if (ImGui::CollapsingHeader(ICON_FA_PALETTE " Graphics", flags)) {
            PropertyTable table{ "Graphics", &context_ };

            table.ConstPropertyUnformatted("Framebuffer cache", std::format("{}", state->framebufferCache.Size()).c_str());
            table.ConstPropertyUnformatted("RTV cache:", std::format("{}", state->rtvCache.Size()).c_str());
            table.ConstPropertyUnformatted("ShadowMap cache:", std::format("{}", state->shadowMapCache.Size()).c_str());
        }

        if (context_.ActiveWorld() && ImGui::CollapsingHeader(ICON_FA_EARTH " Scene", flags)) {
            auto gfxScene{ context_.ActiveWorld()->GetScene<GraphicsScene>() };
            const auto& gfxStats{ gfxScene->GetFrameStats() };
            const auto& gpuStats{ gfxScene->GetFrameGpuStats() };
            const auto gpuTime{ gpuStats.shadowsMS + gpuStats.depthMS + gpuStats.lightCullMS + gpuStats.geometryMS + gpuStats.aoMS + gpuStats.postProcessMS };

            PropertyTable table{ "Scene", &context_ };

            table.ConstPropertyUnformatted("Draw calls", std::format("{}", gfxStats.drawCalls).c_str());
            table.ConstPropertyUnformatted("Compute dispatches", std::format("{}", gfxStats.computeDispatches).c_str());

            drawMs(table, "Shadows", gpuStats.shadowsMS, gpuTime);
            drawMs(table, "Depth", gpuStats.depthMS, gpuTime);
            drawMs(table, "Light cull", gpuStats.lightCullMS, gpuTime);
            drawMs(table, "Geometry", gpuStats.geometryMS, gpuTime);
            drawMs(table, "SSAO", gpuStats.aoMS, gpuTime);
            drawMs(table, "Post process", gpuStats.postProcessMS, gpuTime);

            table.ConstPropertyUnformatted("Frame", std::format("{:0.4f} ms", gpuTime).c_str());
        }
    }

    ImGui::End();
}

namespace {
    EditorToolset::Register _{ [](EditorContext& context) { context.RegisterEditorTool(MakeUnique<StatsWidget>(context.Allocator(), context)); } };
} // namespace

} // namespace ugine::ed