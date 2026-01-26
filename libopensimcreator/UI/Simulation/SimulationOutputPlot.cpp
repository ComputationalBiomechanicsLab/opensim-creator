#include "SimulationOutputPlot.h"

#include <libopensimcreator/Documents/Model/Environment.h>
#include <libopensimcreator/Documents/Simulation/ISimulation.h>
#include <libopensimcreator/Documents/Simulation/SimulationClock.h>
#include <libopensimcreator/Documents/Simulation/SimulationReport.h>
#include <libopensimcreator/Platform/msmicons.h>
#include <libopensimcreator/Platform/OSCColors.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/UI/Simulation/ISimulatorUIAPI.h>

#include <libopynsim/documents/output_extractors/component_output_extractor.h>
#include <libopynsim/documents/output_extractors/component_output_subfield.h>
#include <libopynsim/documents/output_extractors/concatenating_output_extractor.h>
#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/platform/log.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utils/assertions.h>
#include <liboscar/utils/enum_helpers.h>
#include <liboscar/utils/perf.h>
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
    void DrawToggleWatchOutputMenuItem(
        Environment& env,
        const SharedOutputExtractor& output)
    {
        if (env.hasUserOutputExtractor(output)) {
            if (ui::draw_menu_item(MSMICONS_TIMES " Stop Watching")) {
                env.removeUserOutputExtractor(output);
            }
        }
        else {
            if (ui::draw_menu_item(MSMICONS_EYE " Watch Output")) {
                env.addUserOutputExtractor(output);
            }
            ui::draw_tooltip_if_item_hovered("Watch Output", "Watch the selected output. This makes it appear in the 'Output Watches' window in the editor panel and the 'Output Plots' window during a simulation");
        }
    }

    // draw menu items for exporting the output to a CSV
    void DrawExportToCSVMenuItems(
        ISimulatorUIAPI& api,
        const SharedOutputExtractor& output)
    {
        if (ui::draw_menu_item(MSMICONS_SAVE "Save as CSV")) {
            api.tryPromptToSaveOutputsAsCSV({output}, false);
        }

        if (ui::draw_menu_item(MSMICONS_SAVE "Save as CSV (and open)")) {
            api.tryPromptToSaveOutputsAsCSV({output}, true);
        }
    }

    // draw a menu that prompts the user to select some other output
    void DrawSelectOtherOutputMenuContent(
        ISimulation& simulation,
        const SharedOutputExtractor& oneDimensionalOutputExtractor)
    {
        static_assert(num_options<OutputExtractorDataType>() == 3);
        OSC_ASSERT(oneDimensionalOutputExtractor.getOutputType() == OutputExtractorDataType::Float);

        // HACK: pre-acquire the environment, because `*simulation.getModel()` acquires the model
        //       mutex for the entire duration of the `ForEachComponentInclusive`, and acquiring
        //       the environment inside this function then causes a recursion error/deadlock (#969)
        auto environment = simulation.tryUpdEnvironment();

        int id = 0;
        opyn::ForEachComponentInclusive(*simulation.getModel(), [&oneDimensionalOutputExtractor, environment, &id](const auto& component)
        {
            const auto numOutputs = component.getNumOutputs();
            if (numOutputs <= 0) {
                return;
            }

            std::vector<std::reference_wrapper<const OpenSim::AbstractOutput>> extractableOutputs;
            extractableOutputs.reserve(numOutputs);  // upper bound
            for (const auto& [name, output] : component.getOutputs()) {
                if (ProducesExtractableNumericValues(*output)) {
                    extractableOutputs.push_back(*output);
                }
            }

            if (!extractableOutputs.empty()) {
                ui::push_id(id++);
                if (ui::begin_menu(component.getName())) {
                    for (const OpenSim::AbstractOutput& output : extractableOutputs) {
                        ui::push_id(id++);
                        DrawRequestOutputMenuOrMenuItem(output, [&oneDimensionalOutputExtractor, &environment](const SharedOutputExtractor& rhs)
                        {
                            SharedOutputExtractor concatenating = SharedOutputExtractor{ConcatenatingOutputExtractor{oneDimensionalOutputExtractor, rhs}};
                            environment->overwriteOrAddNewUserOutputExtractor(oneDimensionalOutputExtractor, concatenating);
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
    void DrawPlotAgainstOtherOutputMenuItem(
        ISimulation& sim,
        const SharedOutputExtractor& output)
    {
        if (ui::begin_menu(MSMICONS_CHART_LINE "Plot Against Other Output")) {
            DrawSelectOtherOutputMenuContent(sim, output);
            ui::end_menu();
        }
    }

    void TryDrawOutputContextMenuForLastItem(
        ISimulatorUIAPI& api,
        ISimulation& sim,
        const SharedOutputExtractor& output)
    {
        if (not ui::begin_popup_context_menu("outputplotmenu")) {
            return;  // menu not open
        }

        const OutputExtractorDataType dataType = output.getOutputType();

        if (dataType == OutputExtractorDataType::Float) {
            DrawExportToCSVMenuItems(api, output);
            DrawPlotAgainstOtherOutputMenuItem(sim, output);
            DrawToggleWatchOutputMenuItem(*sim.tryUpdEnvironment(), output);
        }
        else if (dataType == OutputExtractorDataType::Vector2) {
            DrawExportToCSVMenuItems(api, output);
            DrawToggleWatchOutputMenuItem(*sim.tryUpdEnvironment(), output);
        }
        else {
            DrawToggleWatchOutputMenuItem(*sim.tryUpdEnvironment(), output);

        }

        ui::end_popup();
    }
}

class osc::SimulationOutputPlot::Impl final {
public:

    Impl(ISimulatorUIAPI* api, SharedOutputExtractor outputExtractor, float height) :
        m_API{api},
        m_OutputExtractor{std::move(outputExtractor)},
        m_Height{height}
    {}

    void onDraw()
    {
        static_assert(num_options<OutputExtractorDataType>() == 3);

        const ptrdiff_t nReports = m_API->updSimulation().getNumReports();
        OutputExtractorDataType outputType = m_OutputExtractor.getOutputType();

        if (nReports <= 0) {
            ui::draw_text("no data (yet)");
        }
        else if (outputType == OutputExtractorDataType::Float) {
            drawFloatOutputUI();
        }
        else if (outputType == OutputExtractorDataType::String) {
            drawStringOutputUI();
        }
        else if (outputType == OutputExtractorDataType::Vector2) {
            drawVector2OutputUI();
        }
        else {
            ui::draw_text("unknown output type");
        }
    }

private:
    void drawFloatOutputUI()
    {
        OSC_ASSERT(m_OutputExtractor.getOutputType() == OutputExtractorDataType::Float && "should've been checked before calling this function");

        ISimulation& sim = m_API->updSimulation();

        const ptrdiff_t nReports = sim.getNumReports();
        if (nReports <= 0) {
            ui::draw_text("no data (yet)");
            return;
        }

        // Collect output data from the output extractor.
        std::vector<float> buf;
        {
            OSC_PERF("collect output data");
            const std::vector<SimulationReport> reports = sim.getAllSimulationReports();
            buf = m_OutputExtractor.slurpValues<float>(*sim.getModel(), reports);
        }

        // setup drawing area for drawing
        ui::set_next_item_width(ui::get_content_region_available().x());
        const float plotWidth = ui::get_content_region_available().x();
        Rect plotRect{};

        // draw the plot
        {
            OSC_PERF("draw output plot");

            plot::push_style_var(plot::PlotStyleVar::PlotPadding, {0.0f, 0.0f});
            plot::push_style_var(plot::PlotStyleVar::PlotBorderSize, 0.0f);
            plot::push_style_var(plot::PlotStyleVar::FitPadding, {0.0f, 1.0f});
            const auto flags =
                plot::PlotFlags::NoTitle |
                plot::PlotFlags::NoLegend |
                plot::PlotFlags::NoMenus |
                plot::PlotFlags::NoBoxSelect |
                plot::PlotFlags::NoFrame |
                plot::PlotFlags::NoMouseText;

            if (plot::begin("##", {plotWidth, m_Height}, flags)) {
                plot::setup_axis(plot::Axis::X1, std::nullopt, plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus | plot::AxisFlags::AutoFit);
                plot::setup_axis(plot::Axis::Y1, std::nullopt, plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus | plot::AxisFlags::AutoFit);
                plot::push_style_color(plot::PlotColorVar::Line, Color::white().with_alpha(0.7f));
                plot::push_style_color(plot::PlotColorVar::PlotBackground, Color::clear());
                plot::plot_line("##", buf);
                plot::pop_style_color();
                plot::pop_style_color();

                plotRect = plot::get_plot_ui_rect();

                plot::end();
            }
            plot::pop_style_var();
            plot::pop_style_var();
            plot::pop_style_var();
        }
        bool plotIsHovered = ui::is_item_hovered();

        // if the user right-clicks, draw a context menu
        TryDrawOutputContextMenuForLastItem(*m_API, sim, m_OutputExtractor);

        // (the rest): handle scrubber overlay
        OSC_PERF("draw output plot overlay");

        // figure out mapping between screen space and plot space

        const SimulationClock::time_point simStartTime = sim.getSimulationReport(0).getTime();
        const SimulationClock::time_point simEndTime = sim.getSimulationReport(nReports-1).getTime();
        const SimulationClock::duration simTimeStep = (simEndTime-simStartTime)/nReports;
        const SimulationClock::time_point simScrubTime = m_API->getSimulationScrubTime();

        const float simScrubPct = static_cast<float>(static_cast<double>((simScrubTime - simStartTime)/(simEndTime - simStartTime)));

        ui::DrawListView drawlist = ui::get_panel_draw_list();
        const Color currentTimeLineColor = OSCColors::scrub_current();
        const Color hoverTimeLineColor = OSCColors::scrub_hovered();

        // draw a vertical Y line showing the current scrub time over the plots
        {
            const float plotScrubLineX = plotRect.left() + simScrubPct*plotRect.width();
            const Vector2 p1 = {plotScrubLineX, plotRect.ypd_top()};
            const Vector2 p2 = {plotScrubLineX, plotRect.ypd_bottom()};
            drawlist.add_line(p1, p2, currentTimeLineColor);
        }

        if (plotIsHovered) {
            const Vector2 mp = ui::get_mouse_ui_position();
            const Vector2 plotLoc = mp - plotRect.ypd_top_left();
            const float relLoc = plotLoc.x() / plotRect.width();
            const SimulationClock::time_point timeLoc = simStartTime + relLoc*(simEndTime - simStartTime);

            // draw vertical line to show current X of their hover
            {
                const Vector2 p1 = {mp.x(), plotRect.ypd_top()};
                const Vector2 p2 = {mp.x(), plotRect.ypd_bottom()};
                drawlist.add_line(p1, p2, hoverTimeLineColor);
            }

            // show a tooltip of X and Y
            {
                const int step = static_cast<int>((timeLoc - simStartTime) / simTimeStep);
                if (0 <= step and static_cast<size_t>(step) < buf.size()) {
                    const float y = buf[static_cast<size_t>(step)];

                    // ensure the tooltip doesn't occlude the line
                    ui::push_style_color(ui::ColorVar::PopupBg, ui::get_style_color(ui::ColorVar::PopupBg).with_alpha(0.5f));
                    ui::set_tooltip("(%.2fs, %.4f)", static_cast<float>(timeLoc.time_since_epoch().count()), y);
                    ui::pop_style_color();
                }
            }

            // if the user presses their left mouse while hovering over the plot,
            // change the current sim scrub time to match their press location
            if (ui::is_mouse_down(ui::MouseButton::Left)) {
                m_API->setSimulationScrubTime(timeLoc);
            }
        }
    }

    void drawStringOutputUI()
    {
        ISimulation& sim = m_API->updSimulation();
        const ptrdiff_t nReports = m_API->updSimulation().getNumReports();
        const SimulationReport r = m_API->trySelectReportBasedOnScrubbing().value_or(sim.getSimulationReport(nReports - 1));

        ui::draw_text_centered(m_OutputExtractor.getValue<std::string>(*sim.getModel(), r));
        TryDrawOutputContextMenuForLastItem(*m_API, sim, m_OutputExtractor);
    }

    void drawVector2OutputUI()
    {
        OSC_ASSERT(m_OutputExtractor.getOutputType() == OutputExtractorDataType::Vector2);

        ISimulation& sim = m_API->updSimulation();

        const ptrdiff_t nReports = sim.getNumReports();
        if (nReports <= 0) {
            ui::draw_text("no data (yet)");
            return;
        }

        // collect output data from the output extractor.
        std::vector<Vector2> buf;
        {
            OSC_PERF("collect output data");
            std::vector<SimulationReport> reports = sim.getAllSimulationReports();
            buf = m_OutputExtractor.slurpValues<Vector2>(*sim.getModel(), reports);
        }

        // setup drawing area for drawing
        ui::set_next_item_width(ui::get_content_region_available().x());
        const float plotWidth = ui::get_content_region_available().x();
        Rect plotRect{};

        // draw the plot
        {
            OSC_PERF("draw output plot");

            plot::push_style_var(plot::PlotStyleVar::PlotPadding, {0.0f, 0.0f});
            plot::push_style_var(plot::PlotStyleVar::PlotBorderSize, 0.0f);
            plot::push_style_var(plot::PlotStyleVar::FitPadding, {0.1f, 0.1f});
            plot::push_style_var(plot::PlotStyleVar::AnnotationPadding, ui::get_style_panel_padding());
            const auto flags = plot::PlotFlags::NoTitle | plot::PlotFlags::NoLegend | plot::PlotFlags::NoMenus | plot::PlotFlags::NoBoxSelect | plot::PlotFlags::NoFrame;

            if (plot::begin("##", {plotWidth, m_Height}, flags)) {
                plot::setup_axis(plot::Axis::X1, std::nullopt, plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus | plot::AxisFlags::AutoFit);
                plot::setup_axis(plot::Axis::Y1, std::nullopt, plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus | plot::AxisFlags::AutoFit);
                plot::push_style_color(plot::PlotColorVar::Line, Color::white().with_alpha(0.7f));
                plot::push_style_color(plot::PlotColorVar::PlotBackground, Color::clear());
                plot::plot_line("##", buf);
                plot::pop_style_color();
                plot::pop_style_color();

                plotRect = plot::get_plot_ui_rect();

                // overlays
                {
                    SimulationReport currentReport = m_API->trySelectReportBasedOnScrubbing().value_or(sim.getSimulationReport(nReports - 1));
                    Vector2d currentVal = m_OutputExtractor.getValue<Vector2>(*sim.getModel(), currentReport);
                    // ensure the annotation doesn't occlude the line too heavily
                    auto annotationColor = ui::get_style_color(ui::ColorVar::PopupBg).with_alpha(0.5f);
                    plot::draw_annotation(currentVal, annotationColor, {10.0f, 10.0f}, true, "(%f, %f)", currentVal.x(), currentVal.y());
                    plot::drag_point(0, &currentVal, OSCColors::scrub_current(), 4.0f, plot::DragToolFlag::NoInputs);
                }

                plot::end();
            }
            plot::pop_style_var();
            plot::pop_style_var();
            plot::pop_style_var();
            plot::pop_style_var();
        }

        // if the user right-clicks the output, show a context menu
        TryDrawOutputContextMenuForLastItem(*m_API, sim, m_OutputExtractor);
    }

    ISimulatorUIAPI* m_API;
    SharedOutputExtractor m_OutputExtractor;
    float m_Height;
};


osc::SimulationOutputPlot::SimulationOutputPlot(
    ISimulatorUIAPI* api,
    SharedOutputExtractor outputExtractor,
    float height) :

    m_Impl{std::make_unique<Impl>(api, std::move(outputExtractor), height)}
{}
osc::SimulationOutputPlot::SimulationOutputPlot(SimulationOutputPlot&&) noexcept = default;
osc::SimulationOutputPlot& osc::SimulationOutputPlot::operator=(SimulationOutputPlot&&) noexcept = default;
osc::SimulationOutputPlot::~SimulationOutputPlot() noexcept = default;
void osc::SimulationOutputPlot::onDraw() { m_Impl->onDraw(); }
