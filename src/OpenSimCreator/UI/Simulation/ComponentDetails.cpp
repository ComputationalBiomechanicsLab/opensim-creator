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
    ui::Dummy({0.0, 2.5f});
    ImGui::Separator();

    // top-level info
    {
        ui::Columns(2);

        ui::Text("getName()");
        ui::NextColumn();
        ui::Text(c.getName());
        ui::NextColumn();

        ui::Text("getAuthors()");
        ui::NextColumn();
        ui::Text(c.getAuthors());
        ui::NextColumn();

        ui::Text("getOwner().getName()");
        ui::NextColumn();
        ui::Text(TryGetOwnerName(c).value_or("N/A (no owner)"));
        ui::NextColumn();

        ui::Text("getAbsolutePath()");
        ui::NextColumn();
        ui::Text(GetAbsolutePathString(c));
        ui::NextColumn();

        ui::Text("getConcreteClassName()");
        ui::NextColumn();
        ui::Text(c.getConcreteClassName());
        ui::NextColumn();

        ui::Text("getNumInputs()");
        ui::NextColumn();
        ui::Text("%i", c.getNumInputs());
        ui::NextColumn();

        ui::Text("getNumOutputs()");
        ui::NextColumn();
        ui::Text("%i", c.getNumOutputs());
        ui::NextColumn();

        ui::Text("getNumSockets()");
        ui::NextColumn();
        ui::Text("%i", c.getNumSockets());
        ui::NextColumn();

        ui::Text("getNumStateVariables()");
        ui::NextColumn();
        ui::Text("%i", c.getNumStateVariables());
        ui::NextColumn();

        ui::Text("getNumProperties()");
        ui::NextColumn();
        ui::Text("%i", c.getNumProperties());
        ui::NextColumn();

        ui::Columns();
    }

    // properties
    if (ImGui::CollapsingHeader("properties"))
    {
        ui::Columns(2);
        for (int i = 0; i < c.getNumProperties(); ++i)
        {
            OpenSim::AbstractProperty const& p = c.getPropertyByIndex(i);
            ui::Text(p.getName());
            ui::NextColumn();
            ui::Text(p.toString());
            ui::NextColumn();
        }
        ui::Columns();
    }

    // state variables
    if (ImGui::CollapsingHeader("state variables"))
    {
        OpenSim::Array<std::string> names = c.getStateVariableNames();
        ui::Columns(2);
        for (int i = 0; i < names.size(); ++i)
        {
            std::string const& name = names[i];

            ui::Text(name);
            ui::NextColumn();
            ui::Text("%f", c.getStateVariableValue(state, name));
            ui::NextColumn();

            ui::Text("%s (deriv)", name.c_str());
            ui::NextColumn();
            ui::Text("%f", c.getStateVariableDerivativeValue(state, name));
            ui::NextColumn();
        }
        ui::Columns();
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
        ui::Columns(2);
        for (std::string const& sn : socknames)
        {
            ui::Text(sn);
            ui::NextColumn();

            std::string const& cp = c.getSocket(sn).getConnecteePath();
            ui::Text(cp);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                rv.type = ResponseType::SelectionChanged;
                rv.ptr = &c.getComponent(cp);
            }
            ui::NextColumn();
        }
        ui::Columns();
    }

    return rv;
}
