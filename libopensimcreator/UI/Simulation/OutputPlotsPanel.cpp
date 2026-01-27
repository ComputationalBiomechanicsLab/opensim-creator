#include "OutputPlotsPanel.h"

#include <libopensimcreator/Documents/Model/Environment.h>
#include <libopensimcreator/Platform/msmicons.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/UI/Simulation/ISimulatorUIAPI.h>
#include <libopensimcreator/UI/Simulation/SimulationOutputPlot.h>

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type_helpers.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/utilities/algorithms.h>

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
            ui::draw_button(MSMICONS_SAVE " Save All " MSMICONS_CARET_DOWN);
            if (ui::begin_popup_context_menu("##exportoptions", ui::PopupFlag::MouseButtonLeft)) {
                if (ui::draw_menu_item("as CSV")) {
                    m_SimulatorUIAPI->tryPromptToSaveAllOutputsAsCSV(m_Environment->getAllUserOutputExtractors());
                }

                if (ui::draw_menu_item("as CSV (and open)")) {
                    m_SimulatorUIAPI->tryPromptToSaveAllOutputsAsCSV(m_Environment->getAllUserOutputExtractors(), true);
                }

                ui::end_popup();
            }
        }

        ui::draw_separator();
        ui::draw_vertical_spacer(5.0f/15.0f);

        for (int i = 0; i < m_Environment->getNumUserOutputExtractors(); ++i) {
            opyn::SharedOutputExtractor output = m_Environment->getUserOutputExtractor(i);

            ui::push_id(i);
            SimulationOutputPlot plot{m_SimulatorUIAPI, output, 128.0f};
            plot.onDraw();

            DrawOutputNameColumn(output, true, m_SimulatorUIAPI->tryGetCurrentSimulationState());
            ui::same_line();
            if (ui::draw_button(MSMICONS_TRASH)) {
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
