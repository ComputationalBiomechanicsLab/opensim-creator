#include "output_plots_panel.h"

#include <libopensimcreator/documents/model/environment.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/simulation/simulator_ui_api.h>
#include <libopensimcreator/ui/simulation/simulation_output_plot.h>

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
    bool is_any_output_exportable_to_csv(const Environment& env)
    {
        for (int i = 0; i < env.get_num_user_output_extractors(); ++i) {
            if (is_numeric(env.get_user_output_extractor(i).output_type())) {
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
        std::string_view panel_name,
        std::shared_ptr<Environment> environment,
        SimulatorUIAPI* api) :

        PanelPrivate{owner, parent, panel_name},
        environment_{std::move(environment)},
        simulator_uiapi_{api}
    {}

    void draw_content()
    {
        if (environment_->get_num_user_output_extractors() <= 0) {
            ui::draw_text_disabled_and_panel_centered("No outputs being watched");
            ui::draw_text_disabled_and_centered("(Right-click a component and 'Watch Output')");
            return;
        }

        if (is_any_output_exportable_to_csv(*environment_)) {
            ui::draw_button(MSMICONS_SAVE " Save All " MSMICONS_CARET_DOWN);
            if (ui::begin_popup_context_menu("##exportoptions", ui::PopupFlag::MouseButtonLeft)) {
                if (ui::draw_menu_item("as CSV")) {
                    simulator_uiapi_->try_prompt_to_save_all_outputs_as_csv(
                        environment_->get_all_user_output_extractors());
                }

                if (ui::draw_menu_item("as CSV (and open)")) {
                    simulator_uiapi_->try_prompt_to_save_all_outputs_as_csv(
                        environment_->get_all_user_output_extractors(), true);
                }

                ui::end_popup();
            }
        }

        ui::draw_separator();
        ui::draw_vertical_spacer(5.0f / 15.0f);

        for (int i = 0; i < environment_->get_num_user_output_extractors(); ++i) {
            opyn::SharedOutputExtractor output = environment_->get_user_output_extractor(i);

            ui::push_id(i);
            SimulationOutputPlot plot{simulator_uiapi_, output, 128.0f};
            plot.on_draw();

            draw_output_name_column(
                output, true, simulator_uiapi_->try_get_current_simulation_state());
            ui::same_line();
            if (ui::draw_button(MSMICONS_TRASH)) {
                environment_->remove_user_output_extractor(output);
                --i;
            }
            ui::pop_id();
        }
    }

private:
    std::shared_ptr<Environment> environment_;
    SimulatorUIAPI* simulator_uiapi_;
};

osc::OutputPlotsPanel::OutputPlotsPanel(
    Widget* parent,
    std::string_view panel_name,
    std::shared_ptr<Environment> environment,
    SimulatorUIAPI* api) :

    Panel{std::make_unique<Impl>(*this, parent, panel_name, std::move(environment), api)}
{}
void osc::OutputPlotsPanel::impl_draw_content()
{
    private_data().draw_content();
}
