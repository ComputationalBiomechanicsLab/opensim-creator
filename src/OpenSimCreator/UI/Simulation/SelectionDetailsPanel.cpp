#include "SelectionDetailsPanel.h"

#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationModelStatePair.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>
#include <OpenSimCreator/UI/Simulation/SimulationOutputPlot.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>

#include <memory>
#include <string_view>
#include <utility>

using namespace osc;

class osc::SelectionDetailsPanel::Impl final : public StandardPanelImpl {
public:
    Impl(
        std::string_view panelName,
        ISimulatorUIAPI* simulatorUIAPI) :

        StandardPanelImpl{panelName},
        m_SimulatorUIAPI{simulatorUIAPI}
    {}

private:
    void impl_draw_content() final
    {
        SimulationModelStatePair* maybeShownState = m_SimulatorUIAPI->tryGetCurrentSimulationState();
        if (not maybeShownState) {
            ui::TextDisabled("(no simulation selected)");
            return;
        }
        SimulationModelStatePair& ms = *maybeShownState;

        const OpenSim::Component* maybeSelected = ms.getSelected();
        if (not maybeSelected) {
            ui::TextDisabled("(nothing selected)");
            return;
        }
        const OpenSim::Component& selected = *maybeSelected;
        const SimTK::State& state = ms.getState();

        // print generic OpenSim::Object/OpenSim::Component stuff
        {
            ui::Columns(2);

            ui::Text("name");
            ui::NextColumn();
            ui::Text(selected.getName());
            ui::NextColumn();

            ui::Text("authors");
            ui::NextColumn();
            ui::Text(selected.getAuthors());
            ui::NextColumn();

            ui::Columns();
        }

        if (ui::CollapsingHeader("properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            ui::Columns(2);
            for (int i = 0; i < selected.getNumProperties(); ++i) {
                const OpenSim::AbstractProperty& prop = selected.getPropertyByIndex(i);
                ui::Text(prop.getName());
                ui::NextColumn();
                ui::Text(prop.toString());
                ui::NextColumn();
            }
            ui::Columns();
        }

        // print outputs (this is probably what the users are more interested in)
        if (ui::CollapsingHeader("outputs")) {
            int id = 0;
            ui::Columns(2);

            for (const auto& [outputName, aoPtr] : selected.getOutputs()) {
                ui::PushID(id++);

                ui::Text(outputName);
                ui::NextColumn();
                SimulationOutputPlot plot{
                    m_SimulatorUIAPI,
                    OutputExtractor{ComponentOutputExtractor{*aoPtr}},
                    ui::GetTextLineHeight(),
                };
                plot.onDraw();
                ui::NextColumn();

                ui::PopID();
            }
            ui::Columns();
        }

        // state variables
        if (ui::CollapsingHeader("state variables")) {
            OpenSim::Array<std::string> names = selected.getStateVariableNames();
            ui::Columns(2);
            for (int i = 0; i < names.size(); ++i)
            {
                const std::string& name = names[i];

                ui::Text(name);
                ui::NextColumn();
                ui::Text("%f", selected.getStateVariableValue(state, name));
                ui::NextColumn();

                ui::Text("%s (deriv)", name.c_str());
                ui::NextColumn();
                ui::Text("%f", selected.getStateVariableDerivativeValue(state, name));
                ui::NextColumn();
            }
            ui::Columns();
        }

        // inputs
        if (ui::CollapsingHeader("inputs")) {
            std::vector<std::string> input_names = selected.getInputNames();
            for (const std::string& inputName : input_names) {
                ui::Text(inputName);
            }
        }

        // sockets
        if (ui::CollapsingHeader("sockets"))
        {
            std::vector<std::string> socknames = GetSocketNames(selected);
            ui::Columns(2);
            for (const std::string& sn : socknames)
            {
                ui::Text(sn);
                ui::NextColumn();

                const std::string& cp = selected.getSocket(sn).getConnecteePath();
                ui::Text(cp);
                ui::NextColumn();
            }
            ui::Columns();
        }

        // debug info (handy for development)
        if (ui::CollapsingHeader("other")) {
            ui::Columns(2);

            ui::Text("getOwner().name()");
            ui::NextColumn();
            ui::Text(TryGetOwnerName(selected).value_or("N/A (no owner)"));
            ui::NextColumn();

            ui::Text("getAbsolutePath()");
            ui::NextColumn();
            ui::Text(GetAbsolutePathString(selected));
            ui::NextColumn();

            ui::Text("getConcreteClassName()");
            ui::NextColumn();
            ui::Text(selected.getConcreteClassName());
            ui::NextColumn();

            ui::Text("getNumInputs()");
            ui::NextColumn();
            ui::Text("%i", selected.getNumInputs());
            ui::NextColumn();

            ui::Text("getNumOutputs()");
            ui::NextColumn();
            ui::Text("%i", selected.getNumOutputs());
            ui::NextColumn();

            ui::Text("getNumSockets()");
            ui::NextColumn();
            ui::Text("%i", selected.getNumSockets());
            ui::NextColumn();

            ui::Text("getNumStateVariables()");
            ui::NextColumn();
            ui::Text("%i", selected.getNumStateVariables());
            ui::NextColumn();

            ui::Text("getNumProperties()");
            ui::NextColumn();
            ui::Text("%i", selected.getNumProperties());
            ui::NextColumn();

            ui::Columns();
        }
    }

    ISimulatorUIAPI* m_SimulatorUIAPI;
};

osc::SelectionDetailsPanel::SelectionDetailsPanel(std::string_view panelName, ISimulatorUIAPI* simulatorUIAPI) :
    m_Impl{std::make_unique<Impl>(panelName, simulatorUIAPI)}
{}

osc::SelectionDetailsPanel::SelectionDetailsPanel(SelectionDetailsPanel&&) noexcept = default;
osc::SelectionDetailsPanel& osc::SelectionDetailsPanel::operator=(SelectionDetailsPanel&&) noexcept = default;
osc::SelectionDetailsPanel::~SelectionDetailsPanel() noexcept = default;

CStringView osc::SelectionDetailsPanel::impl_get_name() const
{
    return m_Impl->name();
}

bool osc::SelectionDetailsPanel::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::SelectionDetailsPanel::impl_open()
{
    m_Impl->open();
}

void osc::SelectionDetailsPanel::impl_close()
{
    m_Impl->close();
}

void osc::SelectionDetailsPanel::impl_on_draw()
{
    m_Impl->on_draw();
}
