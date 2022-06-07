#include "BasicWidgets.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/ComponentOutputExtractor.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/VirtualModelStatePair.hpp"
#include "src/MainUIStateAPI.hpp"

#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentOutput.h>

static void DrawOutputTooltip(OpenSim::AbstractOutput const& o)
{
    ImGui::BeginTooltip();
    ImGui::Text("%s", o.getTypeName().c_str());
    ImGui::EndTooltip();
}

static void DrawOutputWithSubfieldsMenu(osc::MainUIStateAPI& api, OpenSim::AbstractOutput const& o)
{
    int supportedSubfields = osc::GetSupportedSubfields(o);

    // can plot suboutputs
    if (ImGui::BeginMenu(("  " + o.getName()).c_str()))
    {
        for (osc::OutputSubfield f : osc::GetAllSupportedOutputSubfields())
        {
            if (static_cast<int>(f) & supportedSubfields)
            {
                if (ImGui::MenuItem(GetOutputSubfieldLabel(f)))
                {
                    api.addUserOutputExtractor(osc::OutputExtractor{osc::ComponentOutputExtractor{o, f}});
                }
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::IsItemHovered())
    {
        DrawOutputTooltip(o);
    }
}

static void DrawOutputWithNoSubfieldsMenuItem(osc::MainUIStateAPI& api, OpenSim::AbstractOutput const& o)
{
    // can only plot top-level of output

    if (ImGui::MenuItem(("  " + o.getName()).c_str()))
    {
        api.addUserOutputExtractor(osc::OutputExtractor{osc::ComponentOutputExtractor{o}});
    }

    if (ImGui::IsItemHovered())
    {
        DrawOutputTooltip(o);
    }
}

static void DrawRequestOutputMenuOrMenuItem(osc::MainUIStateAPI& api, OpenSim::AbstractOutput const& o)
{
    if (osc::GetSupportedSubfields(o) == static_cast<int>(osc::OutputSubfield::None))
    {
        DrawOutputWithNoSubfieldsMenuItem(api, o);
    }
    else
    {
        DrawOutputWithSubfieldsMenu(api, o);
    }
}


// public API

void osc::DrawComponentHoverTooltip(OpenSim::Component const& hovered)
{
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);

    ImGui::TextUnformatted(hovered.getName().c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("%s", hovered.getConcreteClassName().c_str());

    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void osc::DrawSelectOwnerMenu(osc::VirtualModelStatePair& model, OpenSim::Component const& selected)
{
    if (ImGui::BeginMenu("Select Owner"))
    {
        OpenSim::Component const* c = &selected;
        model.setHovered(nullptr);
        while (c->hasOwner())
        {
            c = &c->getOwner();

            char buf[128];
            std::snprintf(buf, sizeof(buf), "%s (%s)", c->getName().c_str(), c->getConcreteClassName().c_str());

            if (ImGui::MenuItem(buf))
            {
                model.setSelected(c);
            }
            if (ImGui::IsItemHovered())
            {
                model.setHovered(c);
            }
        }
        ImGui::EndMenu();
    }
}

void osc::DrawRequestOutputsMenu(osc::MainUIStateAPI& api, OpenSim::Component const& c)
{
    if (ImGui::BeginMenu("Request Outputs"))
    {
        osc::DrawHelpMarker("Request that these outputs are plotted whenever a simulation is ran. The outputs will appear in the 'outputs' tab on the simulator screen");

        // iterate from the selected component upwards to the root
        int imguiId = 0;
        OpenSim::Component const* p = &c;
        while (p)
        {
            ImGui::PushID(imguiId++);

            ImGui::Dummy({0.0f, 2.0f});
            ImGui::TextDisabled("%s (%s)", p->getName().c_str(), p->getConcreteClassName().c_str());
            ImGui::Separator();

            if (p->getNumOutputs() == 0)
            {
                ImGui::TextDisabled("  (has no outputs)");
            }
            else
            {
                for (auto const& [name, output] : p->getOutputs())
                {
                    DrawRequestOutputMenuOrMenuItem(api, *output);
                }
            }

            ImGui::PopID();

            p = p->hasOwner() ? &p->getOwner() : nullptr;
        }

        ImGui::EndMenu();
    }
}