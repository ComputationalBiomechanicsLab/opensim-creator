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
            ui::draw_text_disabled("(no simulation selected)");
            return;
        }
        SimulationModelStatePair& ms = *maybeShownState;

        const OpenSim::Component* maybeSelected = ms.getSelected();
        if (not maybeSelected) {
            ui:: draw_text_disabled_and_panel_centered("(nothing selected)");
            return;
        }
        const OpenSim::Component& selected = *maybeSelected;
        const SimTK::State& state = ms.getState();

        // print generic OpenSim::Object/OpenSim::Component stuff
        {
            ui::set_num_columns(2);

            ui::draw_text("name");
            ui::next_column();
            ui::draw_text(selected.getName());
            ui::next_column();

            ui::draw_text("authors");
            ui::next_column();
            ui::draw_text(selected.getAuthors());
            ui::next_column();

            ui::set_num_columns();
        }

        if (ui::draw_collapsing_header("properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            ui::set_num_columns(2);
            for (int i = 0; i < selected.getNumProperties(); ++i) {
                const OpenSim::AbstractProperty& prop = selected.getPropertyByIndex(i);
                ui::draw_text(prop.getName());
                ui::next_column();
                ui::draw_text(prop.toString());
                ui::next_column();
            }
            ui::set_num_columns();
        }

        // print outputs (this is probably what the users are more interested in)
        if (ui::draw_collapsing_header("outputs")) {
            int id = 0;
            ui::set_num_columns(2);

            for (const auto& [outputName, aoPtr] : selected.getOutputs()) {
                ui::push_id(id++);

                ui::draw_text(outputName);
                ui::next_column();
                SimulationOutputPlot plot{
                    m_SimulatorUIAPI,
                    OutputExtractor{ComponentOutputExtractor{*aoPtr}},
                    ui::get_text_line_height(),
                };
                plot.onDraw();
                ui::next_column();

                ui::pop_id();
            }
            ui::set_num_columns();
        }

        // state variables
        if (ui::draw_collapsing_header("state variables")) {
            OpenSim::Array<std::string> names = selected.getStateVariableNames();
            ui::set_num_columns(2);
            for (int i = 0; i < names.size(); ++i)
            {
                const std::string& name = names[i];

                ui::draw_text(name);
                ui::next_column();
                ui::draw_text("%f", selected.getStateVariableValue(state, name));
                ui::next_column();

                ui::draw_text("%s (deriv)", name.c_str());
                ui::next_column();
                ui::draw_text("%f", selected.getStateVariableDerivativeValue(state, name));
                ui::next_column();
            }
            ui::set_num_columns();
        }

        // inputs
        if (ui::draw_collapsing_header("inputs")) {
            std::vector<std::string> input_names = selected.getInputNames();
            for (const std::string& inputName : input_names) {
                ui::draw_text(inputName);
            }
        }

        // sockets
        if (ui::draw_collapsing_header("sockets"))
        {
            std::vector<std::string> socknames = GetSocketNames(selected);
            ui::set_num_columns(2);
            for (const std::string& sn : socknames)
            {
                ui::draw_text(sn);
                ui::next_column();

                const std::string& cp = selected.getSocket(sn).getConnecteePath();
                ui::draw_text(cp);
                ui::next_column();
            }
            ui::set_num_columns();
        }

        // debug info (handy for development)
        if (ui::draw_collapsing_header("other")) {
            ui::set_num_columns(2);

            ui::draw_text("getOwner().name()");
            ui::next_column();
            ui::draw_text(TryGetOwnerName(selected).value_or("N/A (no owner)"));
            ui::next_column();

            ui::draw_text("getAbsolutePath()");
            ui::next_column();
            ui::draw_text(GetAbsolutePathString(selected));
            ui::next_column();

            ui::draw_text("getConcreteClassName()");
            ui::next_column();
            ui::draw_text(selected.getConcreteClassName());
            ui::next_column();

            ui::draw_text("getNumInputs()");
            ui::next_column();
            ui::draw_text("%i", selected.getNumInputs());
            ui::next_column();

            ui::draw_text("getNumOutputs()");
            ui::next_column();
            ui::draw_text("%i", selected.getNumOutputs());
            ui::next_column();

            ui::draw_text("getNumSockets()");
            ui::next_column();
            ui::draw_text("%i", selected.getNumSockets());
            ui::next_column();

            ui::draw_text("getNumStateVariables()");
            ui::next_column();
            ui::draw_text("%i", selected.getNumStateVariables());
            ui::next_column();

            ui::draw_text("getNumProperties()");
            ui::next_column();
            ui::draw_text("%i", selected.getNumProperties());
            ui::next_column();

            ui::set_num_columns();
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
