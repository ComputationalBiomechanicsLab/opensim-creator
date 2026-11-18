#include "simulation_output_plot.h"

#include <libopensimcreator/documents/model/environment.h>
#include <libopensimcreator/documents/simulation/abstract_simulation.h>
#include <libopensimcreator/documents/simulation/simulation_clock.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/platform/open_sim_creator_colors.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/simulation/simulator_ui_api.h>

#include <libopynsim/documents/output_extractors/component_output_extractor.h>
#include <libopynsim/documents/output_extractors/component_output_subfield.h>
#include <libopynsim/documents/output_extractors/concatenating_output_extractor.h>
#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/log.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/perf.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace osc;
namespace plot = osc::ui::plot;

namespace
{
    // draw a menu item for toggling watching the output
    void draw_toggle_watch_output_menu_item(
        Environment& env, const opyn::SharedOutputExtractor& output)
    {
        if (env.has_user_output_extractor(output)) {
            if (ui::draw_menu_item(MSMICONS_TIMES " Stop Watching")) {
                env.remove_user_output_extractor(output);
            }
        } else {
            if (ui::draw_menu_item(MSMICONS_EYE " Watch Output")) {
                env.add_user_output_extractor(output);
            }
            ui::draw_tooltip_if_item_hovered(
                "Watch Output",
                "Watch the selected output. This makes it appear in the 'Output Watches' window in the editor panel and the 'Output Plots' window during a simulation");
        }
    }

    // draw menu items for exporting the output to a CSV
    void draw_export_to_csv_menu_items(
        SimulatorUIAPI& api, const opyn::SharedOutputExtractor& output)
    {
        if (ui::draw_menu_item(MSMICONS_SAVE "Save as CSV")) {
            api.try_prompt_to_save_outputs_as_csv({output}, false);
        }

        if (ui::draw_menu_item(MSMICONS_SAVE "Save as CSV (and open)")) {
            api.try_prompt_to_save_outputs_as_csv({output}, true);
        }
    }

    // draw a menu that prompts the user to select some other output
    void draw_select_other_output_menu_content(
        AbstractSimulation& simulation,
        const opyn::SharedOutputExtractor& one_dimensional_output_extractor)
    {
        static_assert(num_options_v<opyn::OutputExtractorDataType> == 3);
        OSC_ASSERT(
            one_dimensional_output_extractor.output_type() == opyn::OutputExtractorDataType::Float);

        // HACK: pre-acquire the environment, because `*simulation.get_model()` acquires the model
        //       mutex for the entire duration of the `for_each_component_inclusive`, and acquiring
        //       the environment inside this function then causes a recursion error/deadlock (#969)
        auto environment = simulation.try_upd_environment();

        int id = 0;
        opyn::for_each_component_inclusive(
            *simulation.get_model(),
            [&one_dimensional_output_extractor, environment, &id](const auto& component) {
                const auto num_outputs = component.getNumOutputs();
                if (num_outputs <= 0) {
                    return;
                }

                std::vector<std::reference_wrapper<const OpenSim::AbstractOutput>>
                    extractable_outputs;
                extractable_outputs.reserve(num_outputs);  // upper bound
                for (const auto& [name, output] : component.getOutputs()) {
                    if (opyn::produces_extractable_numeric_values(*output)) {
                        extractable_outputs.push_back(*output);
                    }
                }

                if (!extractable_outputs.empty()) {
                    ui::push_id(id++);
                    if (ui::begin_menu(component.getName())) {
                        for (const OpenSim::AbstractOutput& output : extractable_outputs) {
                            ui::push_id(id++);
                            draw_request_output_menu_or_menu_item(
                                output,
                                [&one_dimensional_output_extractor,
                                 &environment](const opyn::SharedOutputExtractor& rhs) {
                                    opyn::SharedOutputExtractor concatenating =
                                        opyn::make_output_extractor<
                                            opyn::ConcatenatingOutputExtractor>(
                                            one_dimensional_output_extractor, rhs);
                                    environment->overwrite_or_add_new_user_output_extractor(
                                        one_dimensional_output_extractor, concatenating);
                                });
                            ui::pop_id();
                        }
                        ui::end_menu();
                    }
                    ui::pop_id();
                }
            });
    }

    // draw menu item for plotting one output against another output
    void draw_plot_against_other_output_menu_item(
        AbstractSimulation& sim, const opyn::SharedOutputExtractor& output)
    {
        if (ui::begin_menu(MSMICONS_CHART_LINE "Plot Against Other Output")) {
            draw_select_other_output_menu_content(sim, output);
            ui::end_menu();
        }
    }

    void try_draw_output_context_menu_for_last_item(
        SimulatorUIAPI& api, AbstractSimulation& sim, const opyn::SharedOutputExtractor& output)
    {
        if (not ui::begin_popup_context_menu("outputplotmenu")) {
            return;  // menu not open
        }

        const opyn::OutputExtractorDataType data_type = output.output_type();

        if (data_type == opyn::OutputExtractorDataType::Float) {
            draw_export_to_csv_menu_items(api, output);
            draw_plot_against_other_output_menu_item(sim, output);
            draw_toggle_watch_output_menu_item(*sim.try_upd_environment(), output);
        } else if (data_type == opyn::OutputExtractorDataType::Vector2) {
            draw_export_to_csv_menu_items(api, output);
            draw_toggle_watch_output_menu_item(*sim.try_upd_environment(), output);
        } else {
            draw_toggle_watch_output_menu_item(*sim.try_upd_environment(), output);
        }

        ui::end_popup();
    }
}

class osc::SimulationOutputPlot::Impl final {
public:
    Impl(SimulatorUIAPI* api, opyn::SharedOutputExtractor output_extractor, float height) :
        api_{api},
        output_extractor_{std::move(output_extractor)},
        height_{height}
    {}

    void on_draw()
    {
        static_assert(num_options_v<opyn::OutputExtractorDataType> == 3);

        const ptrdiff_t n_reports = api_->upd_simulation().get_num_reports();
        opyn::OutputExtractorDataType output_type = output_extractor_.output_type();

        if (n_reports <= 0) {
            ui::draw_text("no data (yet)");
        } else if (output_type == opyn::OutputExtractorDataType::Float) {
            draw_float_output_ui();
        } else if (output_type == opyn::OutputExtractorDataType::String) {
            draw_string_output_ui();
        } else if (output_type == opyn::OutputExtractorDataType::Vector2) {
            draw_vector2_output_ui();
        } else {
            ui::draw_text("unknown output type");
        }
    }

private:
    void draw_float_output_ui()
    {
        OSC_ASSERT(
            output_extractor_.output_type() == opyn::OutputExtractorDataType::Float &&
            "should've been checked before calling this function");

        AbstractSimulation& sim = api_->upd_simulation();

        const ptrdiff_t n_reports = sim.get_num_reports();
        if (n_reports <= 0) {
            ui::draw_text("no data (yet)");
            return;
        }

        // Collect output data from the output extractor.
        std::vector<float> buf;
        {
            OSC_PERF("collect output data");
            const std::vector<SimulationReport> reports = sim.get_all_simulation_reports();
            buf = output_extractor_.slurp_values<float>(*sim.get_model(), reports);
        }

        // setup drawing area for drawing
        ui::set_next_item_width(ui::get_content_region_available().x());
        const float plot_width = ui::get_content_region_available().x();
        Rect plot_rect{};

        // draw the plot
        {
            OSC_PERF("draw output plot");

            plot::push_style_var(plot::PlotStyleVar::PlotPadding, {0.0f, 0.0f});
            plot::push_style_var(plot::PlotStyleVar::PlotBorderSize, 0.0f);
            plot::push_style_var(plot::PlotStyleVar::FitPadding, {0.0f, 1.0f});
            const auto flags = plot::PlotFlags::NoTitle |      //
                               plot::PlotFlags::NoLegend |     //
                               plot::PlotFlags::NoMenus |      //
                               plot::PlotFlags::NoBoxSelect |  //
                               plot::PlotFlags::NoFrame |      //
                               plot::PlotFlags::NoMouseText;

            if (plot::begin("##", {plot_width, height_}, flags)) {
                plot::setup_axis(
                    plot::Axis::X1,
                    std::nullopt,
                    plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus |
                        plot::AxisFlags::AutoFit);
                plot::setup_axis(
                    plot::Axis::Y1,
                    std::nullopt,
                    plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus |
                        plot::AxisFlags::AutoFit);
                plot::push_style_color(plot::PlotColorVar::Line, Color::white().with_alpha(0.7f));
                plot::push_style_color(plot::PlotColorVar::PlotBackground, Color::clear());
                plot::plot_line("##", buf);
                plot::pop_style_color();
                plot::pop_style_color();

                plot_rect = plot::get_plot_ui_rect();

                plot::end();
            }
            plot::pop_style_var();
            plot::pop_style_var();
            plot::pop_style_var();
        }
        bool plot_is_hovered = ui::is_item_hovered();

        // if the user right-clicks, draw a context menu
        try_draw_output_context_menu_for_last_item(*api_, sim, output_extractor_);

        // (the rest): handle scrubber overlay
        OSC_PERF("draw output plot overlay");

        // figure out mapping between screen space and plot space

        const SimulationClock::time_point sim_start_time = sim.get_simulation_report(0).get_time();
        const SimulationClock::time_point sim_end_time =
            sim.get_simulation_report(n_reports - 1).get_time();
        const SimulationClock::duration sim_time_step = (sim_end_time - sim_start_time) / n_reports;
        const SimulationClock::time_point sim_scrub_time = api_->get_simulation_scrub_time();

        const float sim_scrub_pct = static_cast<float>(static_cast<double>(
            (sim_scrub_time - sim_start_time) / (sim_end_time - sim_start_time)));

        ui::DrawListView drawlist = ui::get_panel_draw_list();
        const Color current_time_line_color = OSCColors::scrub_current();
        const Color hover_time_line_color = OSCColors::scrub_hovered();

        // draw a vertical Y line showing the current scrub time over the plots
        {
            const float plot_scrub_line_x = plot_rect.left() + sim_scrub_pct * plot_rect.width();
            const Vector2 p1 = {plot_scrub_line_x, plot_rect.ypd_top()};
            const Vector2 p2 = {plot_scrub_line_x, plot_rect.ypd_bottom()};
            drawlist.add_line(p1, p2, current_time_line_color);
        }

        if (plot_is_hovered) {
            const Vector2 mp = ui::get_mouse_ui_position();
            const Vector2 plot_loc = mp - plot_rect.ypd_top_left();
            const float rel_loc = plot_loc.x() / plot_rect.width();
            const SimulationClock::time_point time_loc =
                sim_start_time + rel_loc * (sim_end_time - sim_start_time);

            // draw vertical line to show current X of their hover
            {
                const Vector2 p1 = {mp.x(), plot_rect.ypd_top()};
                const Vector2 p2 = {mp.x(), plot_rect.ypd_bottom()};
                drawlist.add_line(p1, p2, hover_time_line_color);
            }

            // show a tooltip of X and Y
            {
                const int step = static_cast<int>((time_loc - sim_start_time) / sim_time_step);
                if (0 <= step and static_cast<size_t>(step) < buf.size()) {
                    const float y = buf[static_cast<size_t>(step)];

                    // ensure the tooltip doesn't occlude the line
                    ui::push_style_color(
                        ui::ColorVar::PopupBg,
                        ui::get_style_color(ui::ColorVar::PopupBg).with_alpha(0.5f));
                    ui::set_tooltip_fmt(
                        "({:.2f}s, {:.4f})", time_loc.time_since_epoch().count(), y);
                    ui::pop_style_color();
                }
            }

            // if the user presses their left mouse while hovering over the plot,
            // change the current sim scrub time to match their press location
            if (ui::is_mouse_down(MouseButton::Left)) {
                api_->set_simulation_scrub_time(time_loc);
            }
        }
    }

    void draw_string_output_ui()
    {
        AbstractSimulation& sim = api_->upd_simulation();
        const ptrdiff_t n_reports = api_->upd_simulation().get_num_reports();
        const SimulationReport r = api_->try_select_report_based_on_scrubbing().value_or(
            sim.get_simulation_report(n_reports - 1));

        ui::draw_text_centered(output_extractor_.value<std::string>(*sim.get_model(), r));
        try_draw_output_context_menu_for_last_item(*api_, sim, output_extractor_);
    }

    void draw_vector2_output_ui()
    {
        OSC_ASSERT(output_extractor_.output_type() == opyn::OutputExtractorDataType::Vector2);

        AbstractSimulation& sim = api_->upd_simulation();

        const ptrdiff_t n_reports = sim.get_num_reports();
        if (n_reports <= 0) {
            ui::draw_text("no data (yet)");
            return;
        }

        // collect output data from the output extractor.
        std::vector<Vector2> buf;
        {
            OSC_PERF("collect output data");
            std::vector<SimulationReport> reports = sim.get_all_simulation_reports();
            buf = output_extractor_.slurp_values<Vector2>(*sim.get_model(), reports);
        }

        // setup drawing area for drawing
        ui::set_next_item_width(ui::get_content_region_available().x());
        const float plot_width = ui::get_content_region_available().x();
        Rect plot_rect{};

        // draw the plot
        {
            OSC_PERF("draw output plot");

            plot::push_style_var(plot::PlotStyleVar::PlotPadding, {0.0f, 0.0f});
            plot::push_style_var(plot::PlotStyleVar::PlotBorderSize, 0.0f);
            plot::push_style_var(plot::PlotStyleVar::FitPadding, {0.1f, 0.1f});
            plot::push_style_var(
                plot::PlotStyleVar::AnnotationPadding, ui::get_style_panel_padding());
            const auto flags = plot::PlotFlags::NoTitle |      //
                               plot::PlotFlags::NoLegend |     //
                               plot::PlotFlags::NoMenus |      //
                               plot::PlotFlags::NoBoxSelect |  //
                               plot::PlotFlags::NoFrame;

            if (plot::begin("##", {plot_width, height_}, flags)) {
                plot::setup_axis(
                    plot::Axis::X1,
                    std::nullopt,
                    plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus |
                        plot::AxisFlags::AutoFit);
                plot::setup_axis(
                    plot::Axis::Y1,
                    std::nullopt,
                    plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus |
                        plot::AxisFlags::AutoFit);
                plot::push_style_color(plot::PlotColorVar::Line, Color::white().with_alpha(0.7f));
                plot::push_style_color(plot::PlotColorVar::PlotBackground, Color::clear());
                plot::plot_line("##", buf);
                plot::pop_style_color();
                plot::pop_style_color();

                plot_rect = plot::get_plot_ui_rect();

                // overlays
                {
                    SimulationReport current_report =
                        api_->try_select_report_based_on_scrubbing().value_or(
                            sim.get_simulation_report(n_reports - 1));
                    Vector2d current_val =
                        output_extractor_.value<Vector2>(*sim.get_model(), current_report);
                    // ensure the annotation doesn't occlude the line too heavily
                    auto annotation_color =
                        ui::get_style_color(ui::ColorVar::PopupBg).with_alpha(0.5f);
                    plot::draw_annotation(
                        current_val,
                        annotation_color,
                        {10.0f, 10.0f},
                        true,
                        "(%f, %f)",
                        current_val.x(),
                        current_val.y());
                    plot::drag_point(
                        0,
                        &current_val,
                        OSCColors::scrub_current(),
                        4.0f,
                        plot::DragToolFlag::NoInputs);
                }

                plot::end();
            }
            plot::pop_style_var();
            plot::pop_style_var();
            plot::pop_style_var();
            plot::pop_style_var();
        }

        // if the user right-clicks the output, show a context menu
        try_draw_output_context_menu_for_last_item(*api_, sim, output_extractor_);
    }

    SimulatorUIAPI* api_;
    opyn::SharedOutputExtractor output_extractor_;
    float height_;
};

osc::SimulationOutputPlot::SimulationOutputPlot(
    SimulatorUIAPI* api, opyn::SharedOutputExtractor output_extractor, float height) :

    impl_{std::make_unique<Impl>(api, std::move(output_extractor), height)}
{}
osc::SimulationOutputPlot::SimulationOutputPlot(SimulationOutputPlot&&) noexcept = default;
osc::SimulationOutputPlot& osc::SimulationOutputPlot::operator=(SimulationOutputPlot&&) noexcept =
    default;
osc::SimulationOutputPlot::~SimulationOutputPlot() noexcept = default;
void osc::SimulationOutputPlot::on_draw()
{
    impl_->on_draw();
}
