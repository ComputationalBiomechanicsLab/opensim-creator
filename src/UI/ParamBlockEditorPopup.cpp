#include "ParamBlockEditorPopup.hpp"

#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/OpenSimBindings/IntegratorMethod.hpp"
#include "src/Utils/ImGuiHelpers.hpp"

#include <imgui.h>

template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

static bool DrawEditor(osc::ParamBlock& b, int idx, double v)
{
    float fv = static_cast<float>(v);
    if (ImGui::InputFloat("##", &fv))
    {
        b.setValue(idx, static_cast<double>(fv));
        return true;
    }
    else
    {
        return false;
    }
}

static bool DrawEditor(osc::ParamBlock& b, int idx, int v)
{
    if (ImGui::InputInt("##", &v))
    {
        b.setValue(idx, v);
        return true;
    }
    else
    {
        return false;
    }
}

static bool DrawEditor(osc::ParamBlock& b, int idx, osc::IntegratorMethod im)
{
    nonstd::span<char const* const> methodStrings = osc::GetAllIntegratorMethodStrings();
    int method = static_cast<int>(im);

    if (ImGui::Combo("##", &method, methodStrings.data(), static_cast<int>(methodStrings.size())))
    {
        b.setValue(idx, static_cast<osc::IntegratorMethod>(method));
        return true;
    }
    else
    {
        return false;
    }
}

static bool DrawEditor(osc::ParamBlock& b, int idx)
{
    osc::ParamValue v = b.getValue(idx);
    bool rv = false;
    auto handler = Overloaded{
        [&b, &rv, idx](double dv) { rv = DrawEditor(b, idx, dv); },
        [&b, &rv, idx](int iv) { rv = DrawEditor(b, idx, iv); },
        [&b, &rv, idx](osc::IntegratorMethod imv) { rv = DrawEditor(b, idx, imv); },
    };
    std::visit(handler, v);
    return rv;
}

bool osc::ParamBlockEditorPopup::draw(char const* popupName, ParamBlock& b)
{
    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (!ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // modal not showing
        return false;
    }

    bool edited = false;

    ImGui::Columns(2);
    for (int i = 0, len = b.size(); i < len; ++i)
    {
        ImGui::PushID(i);

        ImGui::TextUnformatted(b.getName(i).c_str());
        ImGui::SameLine();
        DrawHelpMarker(b.getName(i).c_str(), b.getDescription(i).c_str());
        ImGui::NextColumn();

        if (DrawEditor(b, i))
        {
            edited = true;
        }
        ImGui::NextColumn();

        ImGui::PopID();
    }
    ImGui::Columns();

    ImGui::Dummy({0.0f, 1.0f});

    if (ImGui::Button("save"))
    {
        ImGui::CloseCurrentPopup();
    }

    return edited;
}
