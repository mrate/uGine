#include "ShaderPreviewWindow.h"

#include "../EditorContext.h"

namespace ugine::ed {

ShaderPreviewWindow::ShaderPreviewWindow(EditorContext& context)
    : EditorTool{ context, false } {
    auto& view{ context_.MainMenu().Get("View") };
    view.AddAction([this] { return ImGui::Checkbox("Shader preview", &visible_); });

    context_.Events().Connect<OpenResourceEvent<Shader>, &ShaderPreviewWindow::OnOpenResource>(this);
    context_.Events().Connect<OpenTypelessResourceEvent, &ShaderPreviewWindow::OnOpenTypelessResource>(this);
}

void ShaderPreviewWindow::Render() {
    if (!shader_) {
        visible_ = false;
        return;
    }

    if (ImGui::Begin("Shader", &visible_)) {
        switch (shader_->State()) {
        case ResourceState::Unloaded: ImGui::TextUnformatted("Not loaded"); break;
        case ResourceState::Loaded: PopulateShader(); break;
        case ResourceState::Loading: ImGui::TextUnformatted("Loading..."); break;
        case ResourceState::Failed:
            {
                ImScope::Color red{ ImGuiCol_Text, ImVec4{ 1, 0, 0, 1 } };

                ImGui::TextUnformatted("Shader error");
            }
            break;
        }
    }

    ImGui::End();
}

void ShaderPreviewWindow::SetShader(ResourceHandle<Shader> shader) {
    shader_ = shader;
    visible_ = true;
}

void ShaderPreviewWindow::OnOpenResource(const OpenResourceEvent<Shader>& event) {
    SetShader(event.resource);
}

void ShaderPreviewWindow::OnOpenTypelessResource(const OpenTypelessResourceEvent& event) {
    if (event.resource.type == Shader::TYPE) {
        SetShader(context_.Engine().GetResources().Get<Shader>(event.resource.id));
    }
}

void ShaderPreviewWindow::PopulateShader() {
    if (ImGui::Button(ICON_FA_RELOAD " Reload")) {
        context_.Events().ReloadResource(shader_);
    }

    ImGui::Text("Name: %s", shader_->Name().Data());
    ImGui::Text("Category: %s", shader_->Category().Data());

    ImGui::Separator();

    ImGui::Text("Variantions: %u", shader_->Variants().size());
    ImGui::Text("Variants mask: %u", shader_->VariantMask());

    auto state{ context_.Engine().GetState<GraphicsState>() };

    {
        ImScope::Indent indent{};
        const auto& variants{ state->shaderVariants.Get() };
        for (u32 i{}; i < variants.Size(); ++i) {
            if ((u32(1) << i) & shader_->VariantMask()) {
                ImGui::Text(" - %s", variants[i].Data());
            }
        }
    }

    ImGui::Separator();

    ImGui::TextUnformatted("Params:");

    {
        ImScope::Indent indent{};
        for (const auto& [name, _] : shader_->Params()) {
            ImGui::TextUnformatted(name.Data());

            auto defaultValue{ shader_->DefaultShaderValue(StringID{ name.Data() }) };
            if (defaultValue) {
                ImGui::SameLine();

                ImGuiEx::UniformValue(*defaultValue);
            }
        }
    }
}

} // namespace ugine::ed