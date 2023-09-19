#include "SelectionDetailsPanel.hpp"

#include <OpenSimCreator/MiddlewareAPIs/SimulatorUIAPI.hpp>
#include <OpenSimCreator/Outputs/ComponentOutputExtractor.hpp>
#include <OpenSimCreator/Simulation/SimulationModelStatePair.hpp>
#include <OpenSimCreator/Widgets/ComponentDetails.hpp>
#include <OpenSimCreator/Widgets/SimulationOutputPlot.hpp>

#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <oscar/UI/Panels/StandardPanel.hpp>

#include <memory>
#include <string_view>
#include <utility>

class osc::SelectionDetailsPanel::Impl final : public StandardPanel {
public:
    Impl(
        std::string_view panelName,
        SimulatorUIAPI* simulatorUIAPI) :

        StandardPanel{panelName},
        m_SimulatorUIAPI{simulatorUIAPI}
    {
    }

private:
    void implDrawContent() final
    {
        SimulationModelStatePair* maybeShownState = m_SimulatorUIAPI->tryGetCurrentSimulationState();

        if (!maybeShownState)
        {
            ImGui::TextDisabled("(no simulation selected)");
            return;
        }

        osc::SimulationModelStatePair& ms = *maybeShownState;

        OpenSim::Component const* selected = ms.getSelected();

        if (!selected)
        {
            ImGui::TextDisabled("(nothing selected)");
            return;
        }

        m_ComponentDetailsWidget.onDraw(ms.getState(), selected);

        if (ImGui::CollapsingHeader("outputs"))
        {
            int imguiID = 0;
            ImGui::Columns(2);
            for (auto const& [outputName, aoPtr] : selected->getOutputs())
            {
                ImGui::PushID(imguiID++);

                ImGui::Text("%s", outputName.c_str());
                ImGui::NextColumn();
                SimulationOutputPlot plot
                {
                    m_SimulatorUIAPI,
                    OutputExtractor{osc::ComponentOutputExtractor{*aoPtr}},
                    ImGui::GetTextLineHeight(),
                };
                plot.onDraw();
                ImGui::NextColumn();

                ImGui::PopID();
            }
            ImGui::Columns();
        }
    }

    SimulatorUIAPI* m_SimulatorUIAPI;
    ComponentDetails m_ComponentDetailsWidget;
};

osc::SelectionDetailsPanel::SelectionDetailsPanel(std::string_view panelName, SimulatorUIAPI* simulatorUIAPI) :
    m_Impl{std::make_unique<Impl>(panelName, simulatorUIAPI)}
{
}

osc::SelectionDetailsPanel::SelectionDetailsPanel(SelectionDetailsPanel&&) noexcept = default;
osc::SelectionDetailsPanel& osc::SelectionDetailsPanel::operator=(SelectionDetailsPanel&&) noexcept = default;
osc::SelectionDetailsPanel::~SelectionDetailsPanel() noexcept = default;

osc::CStringView osc::SelectionDetailsPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::SelectionDetailsPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::SelectionDetailsPanel::implOpen()
{
    m_Impl->open();
}

void osc::SelectionDetailsPanel::implClose()
{
    m_Impl->close();
}

void osc::SelectionDetailsPanel::implOnDraw()
{
    m_Impl->onDraw();
}
