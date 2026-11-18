#include "output_watches_panel.h"

#include <libopensimcreator/documents/model/model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/model/environment.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/platform/msmicons.h>

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/utilities/uid.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    struct CachedSimulationReport final {
        UID sourceModelVersion;
        UID sourceStateVersion;
        SimulationReport simulationReport;
    };

    void update_cached_simulation_report_if_necessary(
        const opyn::ModelStatePair& src, CachedSimulationReport& cache)
    {
        const UID model_version = src.get_model_version();
        const UID state_version = src.get_state_version();

        if (cache.sourceModelVersion == model_version and
            cache.sourceStateVersion == state_version) {
            return;  // it's already up-to-date
        }

        SimTK::State s = src.get_state();
        src.get_model().realizeReport(s);

        cache.simulationReport = SimulationReport{std::move(s)};
        cache.sourceModelVersion = model_version;
        cache.sourceStateVersion = state_version;
    }
}

class osc::OutputWatchesPanel::Impl final : public PanelPrivate {
public:
    explicit Impl(
        OutputWatchesPanel& owner,
        Widget* parent,
        std::string_view panel_name,
        std::shared_ptr<const ModelStatePairWithSharedEnvironment> model) :

        PanelPrivate{owner, parent, panel_name},
        model_{std::move(model)}
    {}

    void draw_content()
    {
        update_cached_simulation_report_if_necessary(*model_, cached_report_);

        std::shared_ptr<Environment> env = model_->try_upd_environment();
        if (env->get_num_user_output_extractors() > 0 and
            ui::begin_table("##OutputWatchesTable", 3, ui::TableFlag::SizingStretchProp)) {

            ui::table_setup_column("##Actions");
            ui::table_setup_column("Output", ui::ColumnFlag::WidthStretch);
            ui::table_setup_column("Value");
            ui::table_headers_row();

            for (int output_idx = 0; output_idx < env->get_num_user_output_extractors();
                 ++output_idx) {

                opyn::SharedOutputExtractor o = env->get_user_output_extractor(output_idx);

                ui::push_id(output_idx);
                ui::table_next_row();

                int column = 0;

                // Column: actions
                ui::table_set_column_index(column++);
                if (ui::draw_button(MSMICONS_TRASH)) {
                    env->remove_user_output_extractor(output_idx);
                }
                ui::draw_tooltip_if_item_hovered("Remove watch");

                // Column: output name
                ui::table_set_column_index(column++);
                ui::draw_text(o.name());
                ui::same_line();
                if (ui::draw_button(MSMICONS_COPY)) {
                    set_clipboard_text(o.name());
                }
                ui::draw_tooltip_if_item_hovered("Copy output name to clipboard");

                // Column: output value
                ui::table_set_column_index(column++);
                const auto value_text =
                    o.value<std::string>(model_->get_model(), cached_report_.simulationReport);
                ui::draw_text(value_text);
                ui::same_line();
                if (ui::draw_button(MSMICONS_COPY)) {
                    set_clipboard_text(value_text);
                }
                ui::draw_tooltip_if_item_hovered("Copy output value to clipboard");

                ui::pop_id();
            }

            ui::end_table();
        } else {
            ui::draw_text_disabled_and_panel_centered("No outputs being watched");
            ui::draw_text_disabled_and_centered("(Right-click a component and 'Watch Output')");
        }
    }

private:
    std::shared_ptr<const ModelStatePairWithSharedEnvironment> model_;
    CachedSimulationReport cached_report_;
};

osc::OutputWatchesPanel::OutputWatchesPanel(
    Widget* parent,
    std::string_view panel_name,
    std::shared_ptr<const ModelStatePairWithSharedEnvironment> model) :

    Panel{std::make_unique<Impl>(*this, parent, panel_name, std::move(model))}
{}
void osc::OutputWatchesPanel::impl_draw_content()
{
    private_data().draw_content();
}
