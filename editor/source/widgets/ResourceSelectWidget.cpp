#include "ResourceSelectWidget.h"

#include "ImGuiEx.h"

using namespace ugine;
using namespace ugine::ed;

namespace ImGuiEx {

bool Filter(StringView filter, StringView name) {
    // TODO:
    std::string lower{ name.Data() };
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    std::string fLower{ filter.Data() };
    std::transform(fLower.begin(), fLower.end(), fLower.begin(), [](unsigned char c) { return std::tolower(c); });
    return lower.find(fLower) != std::string::npos;
}

bool SelectResource(EditorContext& context, const ResourceType& type, ResourceID& selected, SelectResourceFlags flags) {
    static char filterName[128] = { '\0' };

    const auto& selectedName{ context.Engine().GetResources().ResourceName(selected) };
    bool result{};

    const auto size{ ImGui::CalcItemWidth() };
    const auto spacing{ ImGui::GetStyle().ItemSpacing.x };
    const auto padding{ ImGui::GetStyle().FramePadding.x };

    ImGui::BeginGroup();

    int count{};
    if (flags & SelectResourceFlags::LocateButton) {
        ++count;
    }
    if (flags & SelectResourceFlags::EditButton) {
        ++count;
    }
    if (flags & (SelectResourceFlags::ResourceLoading | SelectResourceFlags::ResourceError)) {
        ++count;
    }

    ImGui::PushItemWidth(size - padding - count * (spacing + ImGui::GetFontSize()));

    if (ImGui::BeginCombo("##select_resource", selectedName.Data())) {
        //ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);
        ImGui::InputText("##filter", filterName, sizeof(filterName));
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ERASER)) {
            memset(filterName, 0, sizeof(filterName));
        }

        if (flags & SelectResourceFlags::AllowNull) {
            if (ImGui::Selectable("<none>")) {
                selected = ugine::ResourceID{};
                result = true;
            }
        }

        for (auto&& [id, ref] : context.Engine().GetResources().RegisteredResources()) {
            if (ref.type != type) {
                continue;
            }

            const auto name{ context.Engine().GetResources().ResourceName(id) };
            if (filterName[0] != '\0' && !Filter(filterName, name)) {
                continue;
            }

            if (ImGui::Selectable(name.Empty() ? "n/a" : name.Data(), selectedName == name)) {
                result = true;
                selected = id;
            }
        }

        ImGui::EndCombo();
    }

    if (context.DragAndDrop().DropResource(selected, type)) {
        result = true;
    }

    ImGui::PopItemWidth();

    {
        ImScope::Disabled disabled{ selected.uuid.IsNull() };

        if (flags & (SelectResourceFlags::ResourceLoading | SelectResourceFlags::ResourceError)) {
            ImGui::SameLine();

            if (flags & SelectResourceFlags::ResourceError) {
                ImScope::Color red{ ImGuiCol_Text, ImVec4{ 1, 0, 0, 1 } };

                ImGui::TextUnformatted(ICON_FA_BUG);
            } else {
                ImGuiEx::Loading("Loading", ImGui::GetFontSize() * 0.75f);
            }
        }

        if (flags & SelectResourceFlags::LocateButton) {
            ImGui::SameLine();

            ImGui::TextUnformatted(ICON_FA_FOLDER);
            if (ImGui::IsItemClicked(0)) {
                context.Events().LocateAsset(selected);
            }
        }

        if (flags & SelectResourceFlags::EditButton) {
            ImGui::SameLine();

            ImGui::TextUnformatted(ICON_FA_FILE_EDIT);
            if (ImGui::IsItemClicked(0)) {
                context.Events().OpenTypelessResource(ResourceReference{
                    .id = selected,
                    .type = type,
                });
            }
        }
    }

    ImGui::EndGroup();

    return result;
}

} // namespace ImGuiEx