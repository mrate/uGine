#include "ImportShaderWindow.h"

#include "../EditorContext.h"

#include "../widgets/ImGuiEx.h"
#include "../widgets/ImGuiScope.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/ImGui.h>

#include <ShaderImporter.h>

namespace ugine::ed {

ImportShaderWindow::ImportShaderWindow(EditorContext& context)
    : ModalButtonWindow{ context }
    , context_{ context } {
    context_.Events().Connect<ImportResourceEvent<Shader>, &ImportShaderWindow::OnImportResource>(this);
    context_.Events().Connect<ReloadResourceEvent, &ImportShaderWindow::OnReloadResource>(this);
}

void ImportShaderWindow::Init() {
    try {
        if (!shaderErrors_.Empty()) {
            return;
        }

        SetRightButtons({ Button{
            .enabled = true,
            .name = ICON_FA_FOLDER_OPEN " Import",
            .func = [this] { ImportShader(); },
        } });
    } catch (const std::exception& ex) {
        Emit(ErrorEvent{ ex.what() });
    }
}

void ImportShaderWindow::Show() {
    if (!shaderErrors_.Empty()) {
        ShowErrors();
        return;
    }

    SetRightButtonEnabled(0, !sourcePath_.Empty() && !targetPath_.Empty());

    BeginContent();
    ImGui::Text("Source: %s", sourcePath_.Data());

    context_.DirectorySelector().SelectDirectory("Destination:", targetPath_);
    EndContent();

    Buttons();

    ImGui::Separator();
}

bool ImportShaderWindow::LoadShader(const Path& inputFile, const Path& outputFile, String& importError, Vector<ShaderError>& shaderErrors) const {
    ShaderImporter::Options options{};
    options.inputFile = inputFile.Data();
    options.outputFile = outputFile.Data();
    for (const auto& p : context_.ShaderInludeDirs()) {
        options.includeDirs.push_back(p.Data());
    }

    // TDOO:
    options.warningsAsErrors = true;
    options.verbose = true;
    options.generateDebugInfo = true;

    ShaderImporter importer;
    if (importer.Import(options)) {
        return true;
    }

    if (importer.ShaderErrors().empty()) {
        importError = importer.Error();
    } else {
        for (const auto& error : importer.ShaderErrors()) {
            ShaderError e{};
            e.defines = error.defines;
            e.error = error.message;
            e.filename = Path{ std::filesystem::canonical(error.filename).string().c_str() }; // TODO: [PATH]
            e.line = error.errorLine;
            e.charPos = error.errorChar;

            if (error.defines.empty()) {
                e.variantName = "<empty>";
            } else {
                std::stringstream str;
                for (const auto& [key, val] : error.defines) {
                    str << key << ", ";
                }
                e.variantName = str.str();
            }

            shaderErrors.PushBack(e);
        }
    }

    return false;
}

void ImportShaderWindow::ImportShader() {
    auto [resource, path] = context_.CreateResource<Shader>(targetPath_, sourcePath_.Filename(), sourcePath_);

    String importError{};
    if (LoadShader(sourcePath_, path, importError, shaderErrors_)) {
        context_.Events().CloseModal(this);
        return;
    }

    context_.DeleteResource(resource);

    if (shaderErrors_.Empty()) {
        context_.Events().Error(importError);
    }
}

void ImportShaderWindow::ShowErrors() {
    SetRightButtons({});
    BeginContent();

    if (ImGui::BeginTabBar("Errors")) {
        for (const auto& error : shaderErrors_) {
            if (ImGui::BeginTabItem(error.filename.Filename().Data())) {
                if (ImGui::BeginTabBar("Variant")) {
                    if (ImGui::BeginTabItem(error.variantName.Data())) {
                        {
                            ImScope::Color text{ ImGuiCol_Text, ImVec4{ 1, 1, 0, 1 } };

                            const auto fileLine{ std::format("{}:{}:{}", error.filename.Data(), error.line, error.charPos) };
                            ImGui::TextUnformatted(fileLine.c_str());

                            if (ImGui::IsItemClicked(0)) {
                                context_.RunTextEditor(error.filename.Data(), error.line, error.charPos);
                            }
                        }

                        ImGui::TextUnformatted(error.error.Data());
                        ImGui::TextUnformatted("Defines: ");
                        for (const auto [key, val] : error.defines) {
                            ImGui::SameLine();
                            ImGui::Text("%s=\"%s\", ", key.c_str(), val.c_str());
                        }

                        ImGui::Separator();

                        std::istringstream iss{ error.source.Data() };
                        uint32_t lineNo{ 1 };
                        for (std::string line; std::getline(iss, line); ++lineNo) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0.5f, 0.5f, 0.5f, 1 });
                            ImGui::Text("%04d:", lineNo);
                            ImGui::PopStyleColor();

                            ImGui::SameLine();

                            if (lineNo == error.line) {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1, 0, 0, 1 });
                            }
                            ImGui::TextUnformatted(line.c_str());
                            if (lineNo == error.line) {
                                ImGui::PopStyleColor();
                            }
                        }
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }
    EndContent();

    Buttons();
}

void ImportShaderWindow::ReloadShader(const ResourceID& id) {
    const auto targetPath{ context_.Engine().GetResources().ResourcePath(id) };

    auto meta{ MetaFile::OpenResource(targetPath) };
    const auto sourcePath{ meta.Origin() };

    if (sourcePath.Empty() || !FileSystem::Exists(sourcePath)) {
        UGINE_WARN("Unable to reload shader '{}', missing origin '{}'", id.ToString(), sourcePath.String());
        return;
    }

    UGINE_DEBUG("Import shader '{}' from '{}'", id.ToString(), sourcePath.String());

    auto targetPathBak{ targetPath };
    targetPathBak.ReplaceExtension(".bak");

    String importError{};
    if (!LoadShader(sourcePath, targetPathBak, importError, shaderErrors_)) {
        if (shaderErrors_.Empty()) {
            context_.Events().Error(importError);
        } else {
            // TODO:
            //context_.Events().Error("Shader compilation error.");
            context_.Events().ShowModal(this);
        }

        return;
    }

    // TODO:
    UGINE_DEBUG("Reload shader '{}'", id.ToString());

    //auto shader{ context_.Engine().GetResources().Get<Shader>(id) };
    //shader->Unload();

    // TODO:
    FileSystem::Remove(targetPath);
    FileSystem::Rename(targetPathBak, targetPath);

    context_.Engine().GetResources().Reload<Shader>(id);

    //context_.Engine().GetResources().
    //ImportShader(targetPath, sourcePath);
}

void ImportShaderWindow::OnImportResource(const ImportResourceEvent<Shader>& event) {
    shaderErrors_.Clear();
    sourcePath_ = event.path;
    context_.Events().ShowModal(this);
}

void ImportShaderWindow::OnReloadResource(const ReloadResourceEvent& event) {
    if (event.resource.type == Shader::TYPE) {
        ReloadShader(event.resource.id);
    }
}

} // namespace ugine::ed