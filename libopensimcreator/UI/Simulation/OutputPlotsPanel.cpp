#include "OutputPlotsPanel.h"

#include <libopensimcreator/Documents/Model/Environment.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataTypeHelpers.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/UI/Simulation/ISimulatorUIAPI.h>
#include <libopensimcreator/UI/Simulation/SimulationOutputPlot.h>

#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Platform/os.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Panels/PanelPrivate.h>
#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/LifetimedPtr.h>

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

class osc::OutputPlotsPanel::Impl final : public PanelPrivate {
public:
    explicit Impl(
        OutputPlotsPanel& owner,
        Widget* parent,
        std::string_view panelName_,
        std::shared_ptr<Environment> environment,
        ISimulatorUIAPI* api) :

        PanelPrivate{owner, parent, panelName_},
        m_Environment{std::move(environment)},
        m_SimulatorUIAPI{api}
    {}

    void draw_content()
    {
        if (m_Environment->getNumUserOutputExtractors() <= 0) {
            ui::draw_text_disabled_and_panel_centered("No outputs being watched");
            ui::draw_text_disabled_and_centered("(Right-click a component and 'Watch Output')");
            return;
        }

        if (IsAnyOutputExportableToCSV(*m_Environment)) {
            ui::draw_button(OSC_ICON_SAVE " Save All " OSC_ICON_CARET_DOWN);
            if (ui::begin_popup_context_menu("##exportoptions", ui::PopupFlag::MouseButtonLeft)) {
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

        for (int i = 0; i < m_Environment->getNumUserOutputExtractors(); ++i) {
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

private:
    std::shared_ptr<Environment> m_Environment;
    ISimulatorUIAPI* m_SimulatorUIAPI;
};

osc::OutputPlotsPanel::OutputPlotsPanel(
    Widget* parent,
    std::string_view panelName_,
    std::shared_ptr<Environment> environment,
    ISimulatorUIAPI* api) :

    Panel{std::make_unique<Impl>(*this, parent, panelName_, std::move(environment), api)}
{}
void osc::OutputPlotsPanel::impl_draw_content() { private_data().draw_content(); }
