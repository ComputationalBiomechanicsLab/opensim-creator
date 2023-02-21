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
        std::string_view panelName_,
        std::weak_ptr<MainUIStateAPI> mainUIStateAPI_,
        SimulatorUIAPI* simulatorUIAPI_) :

        StandardPanel{std::move(panelName_)},
        m_API{std::move(mainUIStateAPI_)},
        m_SimulatorUIAPI{simulatorUIAPI_}
    {
    }
private:
    void implDrawContent() final
    {
        std::shared_ptr<MainUIStateAPI> api = m_API.lock();

        if (api->getNumUserOutputExtractors() <= 0)
        {
            ImGui::TextDisabled("(no outputs requested)");
            return;
        }

        if (IsAnyOutputExportableToCSV(*api))
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

        for (int i = 0; i < api->getNumUserOutputExtractors(); ++i)
        {
            osc::OutputExtractor output = api->getUserOutputExtractor(i);

            ImGui::PushID(i);
            SimulationOutputPlot plot{m_SimulatorUIAPI, output, 64.0f};
            plot.draw();
            DrawOutputNameColumn(output, true, m_SimulatorUIAPI->tryGetCurrentSimulationState());
            ImGui::PopID();
        }
    }

    std::weak_ptr<MainUIStateAPI> m_API;
    SimulatorUIAPI* m_SimulatorUIAPI;
};

osc::OutputPlotsPanel::OutputPlotsPanel(
    std::string_view panelName_,
    std::weak_ptr<MainUIStateAPI> mainUIStateAPI_,
    SimulatorUIAPI* simulatorUIAPI_) :

    m_Impl{std::make_unique<Impl>(std::move(panelName_), std::move(mainUIStateAPI_), simulatorUIAPI_)}
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