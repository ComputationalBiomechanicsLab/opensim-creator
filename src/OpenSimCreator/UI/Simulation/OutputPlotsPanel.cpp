#include "OutputPlotsPanel.h"

#include <OpenSimCreator/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>
#include <OpenSimCreator/UI/Simulation/SimulationOutputPlot.h>

#include <IconsFontAwesome5.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/Utils/ParentPtr.h>

#include <memory>
#include <string_view>

using namespace osc;

namespace
{
    bool IsAnyOutputExportableToCSV(IMainUIStateAPI& api)
    {
        for (int i = 0; i < api.getNumUserOutputExtractors(); ++i)
        {
            if (api.getUserOutputExtractor(i).getOutputType() == OutputExtractorDataType::Float)
            {
                return true;
            }
        }
        return false;
    }
}

class osc::OutputPlotsPanel::Impl final : public StandardPanelImpl {
public:
    Impl(
        std::string_view panelName_,
        ParentPtr<IMainUIStateAPI> const& mainUIStateAPI_,
        ISimulatorUIAPI* simulatorUIAPI_) :

        StandardPanelImpl{panelName_},
        m_API{mainUIStateAPI_},
        m_SimulatorUIAPI{simulatorUIAPI_}
    {
    }
private:
    void implDrawContent() final
    {
        if (m_API->getNumUserOutputExtractors() <= 0)
        {
            ui::TextDisabled("(no outputs requested)");
            return;
        }

        if (IsAnyOutputExportableToCSV(*m_API))
        {
            ui::Button(ICON_FA_SAVE " Save All " ICON_FA_CARET_DOWN);
            if (ui::BeginPopupContextItem("##exportoptions", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ui::MenuItem("as CSV"))
                {
                    TryPromptAndSaveAllUserDesiredOutputsAsCSV(*m_SimulatorUIAPI);
                }

                if (ui::MenuItem("as CSV (and open)"))
                {
                    auto p = TryPromptAndSaveAllUserDesiredOutputsAsCSV(*m_SimulatorUIAPI);
                    if (!p.empty())
                    {
                        OpenPathInOSDefaultApplication(p);
                    }
                }

                ui::EndPopup();
            }
        }

        ui::Separator();
        ui::Dummy({0.0f, 5.0f});

        for (int i = 0; i < m_API->getNumUserOutputExtractors(); ++i)
        {
            OutputExtractor output = m_API->getUserOutputExtractor(i);

            ui::PushID(i);
            SimulationOutputPlot plot{m_SimulatorUIAPI, output, 128.0f};
            plot.onDraw();
            DrawOutputNameColumn(output, true, m_SimulatorUIAPI->tryGetCurrentSimulationState());
            ui::PopID();
        }
    }

    ParentPtr<IMainUIStateAPI> m_API;
    ISimulatorUIAPI* m_SimulatorUIAPI;
};

osc::OutputPlotsPanel::OutputPlotsPanel(
    std::string_view panelName_,
    ParentPtr<IMainUIStateAPI> const& mainUIStateAPI_,
    ISimulatorUIAPI* simulatorUIAPI_) :

    m_Impl{std::make_unique<Impl>(panelName_, mainUIStateAPI_, simulatorUIAPI_)}
{
}

osc::OutputPlotsPanel::OutputPlotsPanel(OutputPlotsPanel&&) noexcept = default;
osc::OutputPlotsPanel& osc::OutputPlotsPanel::operator=(OutputPlotsPanel&&) noexcept = default;
osc::OutputPlotsPanel::~OutputPlotsPanel() noexcept = default;

CStringView osc::OutputPlotsPanel::implGetName() const
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
