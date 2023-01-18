#include "OutputPlotsPanel.hpp"

#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/SimulatorUIAPI.hpp"
#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/Widgets/SimulationOutputPlot.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/Panels/StandardPanel.hpp"
#include "src/Platform/os.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>

#include <string_view>
#include <memory>
#include <utility>

namespace
{
    bool IsAnyOutputExportableToCSV(osc::MainUIStateAPI& api)
    {
        for (int i = 0; i < api.getNumUserOutputExtractors(); ++i)
        {
            if (api.getUserOutputExtractor(i).getOutputType() == osc::OutputType::Float)
            {
                return true;
            }
        }
        return false;
    }
}

class osc::OutputPlotsPanel::Impl final : public StandardPanel {
public:
    Impl(
        std::string_view panelName,
        MainUIStateAPI* mainUIStateAPI,
        SimulatorUIAPI* simulatorUIAPI) :

        StandardPanel{std::move(panelName)},
        m_API{mainUIStateAPI},
        m_SimulatorUIAPI{simulatorUIAPI}
    {
    }
private:
    void implDrawContent() final
    {
        if (m_API->getNumUserOutputExtractors() <= 0)
        {
            ImGui::TextDisabled("(no outputs requested)");
            return;
        }

        if (IsAnyOutputExportableToCSV(*m_API))
        {
            ImGui::Button(ICON_FA_SAVE " Save All " ICON_FA_CARET_DOWN);
            if (ImGui::BeginPopupContextItem("##exportoptions", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("as CSV"))
                {
                    osc::TryPromptAndSaveAllUserDesiredOutputsAsCSV(*m_SimulatorUIAPI);
                }

                if (ImGui::MenuItem("as CSV (and open)"))
                {
                    auto p = osc::TryPromptAndSaveAllUserDesiredOutputsAsCSV(*m_SimulatorUIAPI);
                    if (!p.empty())
                    {
                        osc::OpenPathInOSDefaultApplication(p);
                    }
                }

                ImGui::EndPopup();
            }
        }

        ImGui::Separator();
        ImGui::Dummy({0.0f, 5.0f});

        for (int i = 0; i < m_API->getNumUserOutputExtractors(); ++i)
        {
            osc::OutputExtractor output = m_API->getUserOutputExtractor(i);

            ImGui::PushID(i);
            SimulationOutputPlot plot{m_SimulatorUIAPI, output, 64.0f};
            plot.draw();
            DrawOutputNameColumn(output, true, m_SimulatorUIAPI->tryGetCurrentSimulationState());
            ImGui::PopID();
        }
    }

    MainUIStateAPI* m_API;
    SimulatorUIAPI* m_SimulatorUIAPI;
};

osc::OutputPlotsPanel::OutputPlotsPanel(
    std::string_view panelName,
    MainUIStateAPI* mainUIStateAPI,
    SimulatorUIAPI* simulatorUIAPI) :

    m_Impl{std::make_unique<Impl>(std::move(panelName), mainUIStateAPI, simulatorUIAPI)}
{
}

osc::OutputPlotsPanel::OutputPlotsPanel(OutputPlotsPanel&&) noexcept = default;
osc::OutputPlotsPanel& osc::OutputPlotsPanel::operator=(OutputPlotsPanel&&) noexcept = default;
osc::OutputPlotsPanel::~OutputPlotsPanel() noexcept = default;

osc::CStringView osc::OutputPlotsPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::OutputPlotsPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::OutputPlotsPanel::implOpen()
{
    m_Impl->open();
}

void osc::OutputPlotsPanel::implClose()
{
    m_Impl->close();
}

void osc::OutputPlotsPanel::implDraw()
{
    m_Impl->draw();
}