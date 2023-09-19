#include "OutputPlotsPanel.hpp"

#include <OpenSimCreator/Outputs/OutputExtractor.hpp>
#include <OpenSimCreator/UI/Middleware/MainUIStateAPI.hpp>
#include <OpenSimCreator/UI/Middleware/SimulatorUIAPI.hpp>
#include <OpenSimCreator/UI/Widgets/BasicWidgets.hpp>
#include <OpenSimCreator/UI/Widgets/SimulationOutputPlot.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <oscar/Platform/os.hpp>
#include <oscar/UI/Panels/StandardPanel.hpp>
#include <oscar/Utils/ParentPtr.hpp>

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
        std::string_view panelName_,
        ParentPtr<MainUIStateAPI> const& mainUIStateAPI_,
        SimulatorUIAPI* simulatorUIAPI_) :

        StandardPanel{panelName_},
        m_API{mainUIStateAPI_},
        m_SimulatorUIAPI{simulatorUIAPI_}
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
            plot.onDraw();
            DrawOutputNameColumn(output, true, m_SimulatorUIAPI->tryGetCurrentSimulationState());
            ImGui::PopID();
        }
    }

    ParentPtr<MainUIStateAPI> m_API;
    SimulatorUIAPI* m_SimulatorUIAPI;
};

osc::OutputPlotsPanel::OutputPlotsPanel(
    std::string_view panelName_,
    ParentPtr<MainUIStateAPI> const& mainUIStateAPI_,
    SimulatorUIAPI* simulatorUIAPI_) :

    m_Impl{std::make_unique<Impl>(panelName_, mainUIStateAPI_, simulatorUIAPI_)}
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

void osc::OutputPlotsPanel::implOnDraw()
{
    m_Impl->onDraw();
}
