#include "ComponentDetails.h"

#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Array.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <oscar/UI/oscimgui.h>

#include <string>
#include <vector>

using namespace osc;

ComponentDetails::Response osc::ComponentDetails::onDraw(
    SimTK::State const& state,
    OpenSim::Component const* comp)
{
    Response rv;

    if (!comp)
    {
        ui::Text("(nothing selected)");
        return rv;
    }

    OpenSim::Component const& c = *comp;

    ui::Text("selection information:");
    ImGui::Dummy({0.0, 2.5f});
    ImGui::Separator();

    // top-level info
    {
        ImGui::Columns(2);

        ui::Text("getName()");
        ImGui::NextColumn();
        ui::Text(c.getName());
        ImGui::NextColumn();

        ui::Text("getAuthors()");
        ImGui::NextColumn();
        ui::Text(c.getAuthors());
        ImGui::NextColumn();

        ui::Text("getOwner().getName()");
        ImGui::NextColumn();
        ui::Text(TryGetOwnerName(c).value_or("N/A (no owner)"));
        ImGui::NextColumn();

        ui::Text("getAbsolutePath()");
        ImGui::NextColumn();
        ui::Text(GetAbsolutePathString(c));
        ImGui::NextColumn();

        ui::Text("getConcreteClassName()");
        ImGui::NextColumn();
        ui::Text(c.getConcreteClassName());
        ImGui::NextColumn();

        ui::Text("getNumInputs()");
        ImGui::NextColumn();
        ui::Text("%i", c.getNumInputs());
        ImGui::NextColumn();

        ui::Text("getNumOutputs()");
        ImGui::NextColumn();
        ui::Text("%i", c.getNumOutputs());
        ImGui::NextColumn();

        ui::Text("getNumSockets()");
        ImGui::NextColumn();
        ui::Text("%i", c.getNumSockets());
        ImGui::NextColumn();

        ui::Text("getNumStateVariables()");
        ImGui::NextColumn();
        ui::Text("%i", c.getNumStateVariables());
        ImGui::NextColumn();

        ui::Text("getNumProperties()");
        ImGui::NextColumn();
        ui::Text("%i", c.getNumProperties());
        ImGui::NextColumn();

        ImGui::Columns();
    }

    // properties
    if (ImGui::CollapsingHeader("properties"))
    {
        ImGui::Columns(2);
        for (int i = 0; i < c.getNumProperties(); ++i)
        {
            OpenSim::AbstractProperty const& p = c.getPropertyByIndex(i);
            ui::Text(p.getName());
            ImGui::NextColumn();
            ui::Text(p.toString());
            ImGui::NextColumn();
        }
        ImGui::Columns();
    }

    // state variables
    if (ImGui::CollapsingHeader("state variables"))
    {
        OpenSim::Array<std::string> names = c.getStateVariableNames();
        ImGui::Columns(2);
        for (int i = 0; i < names.size(); ++i)
        {
            std::string const& name = names[i];

            ui::Text(name);
            ImGui::NextColumn();
            ui::Text("%f", c.getStateVariableValue(state, name));
            ImGui::NextColumn();

            ui::Text("%s (deriv)", name.c_str());
            ImGui::NextColumn();
            ui::Text("%f", c.getStateVariableDerivativeValue(state, name));
            ImGui::NextColumn();
        }
        ImGui::Columns();
    }

    // inputs
    if (ImGui::CollapsingHeader("inputs"))
    {
        std::vector<std::string> input_names = c.getInputNames();
        for (std::string const& inputName : input_names)
        {
            ui::Text(inputName);
        }
    }

    // sockets
    if (ImGui::CollapsingHeader("sockets"))
    {
        std::vector<std::string> socknames = GetSocketNames(c);
        ImGui::Columns(2);
        for (std::string const& sn : socknames)
        {
            ui::Text(sn);
            ImGui::NextColumn();

            std::string const& cp = c.getSocket(sn).getConnecteePath();
            ui::Text(cp);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                rv.type = ResponseType::SelectionChanged;
                rv.ptr = &c.getComponent(cp);
            }
            ImGui::NextColumn();
        }
        ImGui::Columns();
    }

    return rv;
}
