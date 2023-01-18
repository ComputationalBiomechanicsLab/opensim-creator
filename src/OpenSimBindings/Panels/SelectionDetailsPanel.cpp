#include "SelectionDetailsPanel.hpp"

#include "src/OpenSimBindings/MiddlewareAPIs/SimulatorUIAPI.hpp"
#include "src/OpenSimBindings/Widgets/ComponentDetails.hpp"
#include "src/OpenSimBindings/Widgets/SimulationOutputPlot.hpp"
#include "src/OpenSimBindings/ComponentOutputExtractor.hpp"
#include "src/OpenSimBindings/SimulationModelStatePair.hpp"
#include "src/Panels/StandardPanel.hpp"

#include <imgui.h>
#include <OpenSim/Common/Component.h>

#include <memory>
#include <string_view>
#include <utility>

class osc::SelectionDetailsPanel::Impl final : public StandardPanel {
public:
    Impl(
        std::string_view panelName,
        SimulatorUIAPI* simulatorUIAPI) :

        StandardPanel{std::move(panelName)},
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

        m_ComponentDetailsWidget.draw(ms.getState(), selected);

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
                plot.draw();
                ImGui::NextColumn();

                ImGui::PopID();
            }
            ImGui::Columns();
        }
    }

private:
    SimulatorUIAPI* m_SimulatorUIAPI;
    ComponentDetails m_ComponentDetailsWidget;
};

osc::SelectionDetailsPanel::SelectionDetailsPanel(std::string_view panelName, SimulatorUIAPI* simulatorUIAPI) :
    m_Impl{std::make_unique<Impl>(std::move(panelName), simulatorUIAPI)}
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

void osc::SelectionDetailsPanel::implDraw()
{
    m_Impl->draw();
}
