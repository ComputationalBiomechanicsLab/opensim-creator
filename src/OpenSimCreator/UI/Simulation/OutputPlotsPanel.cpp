#include "OutputPlotsPanel.h"

#include <OpenSimCreator/Documents/Model/Environment.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractorDataTypeHelpers.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>
#include <OpenSimCreator/UI/Simulation/SimulationOutputPlot.h>

#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/LifetimedPtr.h>

#include <memory>
#include <string_view>

using namespace osc;

namespace
{
    bool IsAnyOutputExportableToCSV(const Environment& env)
    {
        for (int i = 0; i < env.getNumUserOutputExtractors(); ++i) {
            if (is_numeric(env.getUserOutputExtractor(i).getOutputType())) {
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
        std::shared_ptr<Environment> environment,
        ISimulatorUIAPI* api) :

        StandardPanelImpl{panelName_},
        m_Environment{std::move(environment)},
        m_SimulatorUIAPI{api}
    {}
private:
    void impl_draw_content() final
    {
        if (m_Environment->getNumUserOutputExtractors() <= 0)
        {
            ui::draw_text_disabled_and_panel_centered("No outputs being watched");
            ui::draw_text_disabled_and_centered("(Right-click a component and 'Watch Output')");
            return;
        }

        if (IsAnyOutputExportableToCSV(*m_Environment))
        {
            ui::draw_button(OSC_ICON_SAVE " Save All " OSC_ICON_CARET_DOWN);
            if (ui::begin_popup_context_menu("##exportoptions", ui::PopupFlag::MouseButtonLeft))
            {
                if (ui::draw_menu_item("as CSV")) {
                    m_SimulatorUIAPI->tryPromptToSaveAllOutputsAsCSV(m_Environment->getAllUserOutputExtractors());
                }

                if (ui::draw_menu_item("as CSV (and open)")) {
                    if (const auto path = m_SimulatorUIAPI->tryPromptToSaveAllOutputsAsCSV(m_Environment->getAllUserOutputExtractors())) {
                        open_file_in_os_default_application(*path);
                    }
                }

                ui::end_popup();
            }
        }

        ui::draw_separator();
        ui::draw_dummy({0.0f, 5.0f});

        for (int i = 0; i < m_Environment->getNumUserOutputExtractors(); ++i)
        {
            OutputExtractor output = m_Environment->getUserOutputExtractor(i);

            ui::push_id(i);
            SimulationOutputPlot plot{m_SimulatorUIAPI, output, 128.0f};
            plot.onDraw();

            DrawOutputNameColumn(output, true, m_SimulatorUIAPI->tryGetCurrentSimulationState());
            ui::same_line();
            if (ui::draw_button(OSC_ICON_TRASH)) {
                m_Environment->removeUserOutputExtractor(output);
                --i;
            }
            ui::pop_id();
        }
    }

    std::shared_ptr<Environment> m_Environment;
    ISimulatorUIAPI* m_SimulatorUIAPI;
};

osc::OutputPlotsPanel::OutputPlotsPanel(
    std::string_view panelName_,
    std::shared_ptr<Environment> environment,
    ISimulatorUIAPI* api) :

    m_Impl{std::make_unique<Impl>(panelName_, std::move(environment), api)}
{}
osc::OutputPlotsPanel::OutputPlotsPanel(OutputPlotsPanel&&) noexcept = default;
osc::OutputPlotsPanel& osc::OutputPlotsPanel::operator=(OutputPlotsPanel&&) noexcept = default;
osc::OutputPlotsPanel::~OutputPlotsPanel() noexcept = default;

CStringView osc::OutputPlotsPanel::impl_get_name() const
{
    return m_Impl->name();
}

bool osc::OutputPlotsPanel::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::OutputPlotsPanel::impl_open()
{
    m_Impl->open();
}

void osc::OutputPlotsPanel::impl_close()
{
    m_Impl->close();
}

void osc::OutputPlotsPanel::impl_on_draw()
{
    m_Impl->on_draw();
}
