#include "SelectionDetailsPanel.h"

#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationModelStatePair.h>
#include <OpenSimCreator/UI/Simulation/ComponentDetails.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>
#include <OpenSimCreator/UI/Simulation/SimulationOutputPlot.h>

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
        if (!maybeShownState) {
            ui::TextDisabled("(no simulation selected)");
            return;
        }

        SimulationModelStatePair& ms = *maybeShownState;

        OpenSim::Component const* selected = ms.getSelected();
        if (!selected) {
            ui::TextDisabled("(nothing selected)");
            return;
        }

        m_ComponentDetailsWidget.onDraw(ms.getState(), selected);

        if (ui::CollapsingHeader("outputs")) {
            int id = 0;
            ui::Columns(2);

            for (auto const& [outputName, aoPtr] : selected->getOutputs()) {
                ui::PushID(id++);

                ui::Text(outputName);
                ui::NextColumn();
                SimulationOutputPlot plot
                {
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
    }

    ISimulatorUIAPI* m_SimulatorUIAPI;
    ComponentDetails m_ComponentDetailsWidget;
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
