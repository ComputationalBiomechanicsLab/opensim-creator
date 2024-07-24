#include "SimulationOutputPlot.h"

#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputSubfield.h>
#include <OpenSimCreator/Documents/OutputExtractors/ConcatenatingOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/ISimulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <oscar/UI/oscimgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/Perf.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace osc;
namespace plot = osc::ui::plot;

namespace
{
    constexpr Color c_CurrentScubTimeColor = Color::yellow().with_alpha(0.6f);
    constexpr Color c_HoveredScrubTimeColor = Color::yellow().with_alpha(0.3f);

    // draw a menu item for toggling watching the output
    void DrawToggleWatchOutputMenuItem(
        ISimulatorUIAPI& api,
        const OutputExtractor& output)
    {
        if (api.hasUserOutputExtractor(output)) {
            if (ui::draw_menu_item(ICON_FA_TRASH " Stop Watching")) {
                api.removeUserOutputExtractor(output);
            }
        }
        else {
            if (ui::draw_menu_item(ICON_FA_EYE " Watch Output")) {
                api.addUserOutputExtractor(output);
            }
            ui::draw_tooltip_if_item_hovered("Watch Output", "Watch the selected output. This makes it appear in the 'Output Watches' window in the editor panel and the 'Output Plots' window during a simulation");
        }
    }

    // draw menu items for exporting the output to a CSV
    void DrawExportToCSVMenuItems(
        ISimulatorUIAPI& api,
        const OutputExtractor& output)
    {
        if (ui::draw_menu_item(ICON_FA_SAVE "Save as CSV")) {
            api.tryPromptToSaveOutputsAsCSV({output});
        }

        if (ui::draw_menu_item(ICON_FA_SAVE "Save as CSV (and open)")) {
            if (const auto path = api.tryPromptToSaveOutputsAsCSV({output})) {
                open_file_in_os_default_application(*path);
            }
        }
    }

    // draw a menu that prompts the user to select some other output
    void DrawSelectOtherOutputMenuContent(
        ISimulatorUIAPI& api,
        ISimulation& simulation,
        const OutputExtractor& oneDimensionalOutputExtractor)
    {
        static_assert(num_options<OutputExtractorDataType>() == 3);
        OSC_ASSERT(oneDimensionalOutputExtractor.getOutputType() == OutputExtractorDataType::Float);

        int id = 0;
        ForEachComponentInclusive(*simulation.getModel(), [&](const auto& component)
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
                        DrawRequestOutputMenuOrMenuItem(output, [&api, &oneDimensionalOutputExtractor](const OpenSim::AbstractOutput& ao, std::optional<ComponentOutputSubfield> subfield)
                        {
                            OutputExtractor rhs = subfield ? OutputExtractor{ComponentOutputExtractor{ao, *subfield}} : OutputExtractor{ComponentOutputExtractor{ao}};
                            OutputExtractor concatenating = OutputExtractor{ConcatenatingOutputExtractor{oneDimensionalOutputExtractor, rhs}};
                            api.overwriteOrAddNewUserOutputExtractor(oneDimensionalOutputExtractor, concatenating);
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
        ISimulatorUIAPI& api,
        ISimulation& sim,
        const OutputExtractor& output)
    {
        if (ui::begin_menu(ICON_FA_CHART_LINE "Plot Against Other Output")) {
            DrawSelectOtherOutputMenuContent(api, sim, output);
            ui::end_menu();
        }
    }

    void TryDrawOutputContextMenuForLastItem(
        ISimulatorUIAPI& api,
        ISimulation& sim,
        const OutputExtractor& output)
    {
        if (not ui::begin_popup_context_menu("outputplotmenu")) {
            return;  // menu not open
        }

        const OutputExtractorDataType dataType = output.getOutputType();

        if (dataType == OutputExtractorDataType::Float) {
            DrawExportToCSVMenuItems(api, output);
            DrawPlotAgainstOtherOutputMenuItem(api, sim, output);
            DrawToggleWatchOutputMenuItem(api, output);
        }
        else if (dataType == OutputExtractorDataType::Vec2) {
            DrawExportToCSVMenuItems(api, output);
            DrawToggleWatchOutputMenuItem(api, output);
        }
        else {
            DrawToggleWatchOutputMenuItem(api, output);

        }

        ui::end_popup();
    }
}

class osc::SimulationOutputPlot::Impl final {
public:

    Impl(ISimulatorUIAPI* api, OutputExtractor outputExtractor, float height) :
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
        else if (outputType == OutputExtractorDataType::Vec2) {
            drawVec2OutputUI();
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

        // collect output data from the `OutputExtractor`
        std::vector<float> buf;
        {
            OSC_PERF("collect output data");
            const std::vector<SimulationReport> reports = sim.getAllSimulationReports();
            buf = m_OutputExtractor.slurpValuesFloat(*sim.getModel(), reports);
        }

        // setup drawing area for drawing
        ui::set_next_item_width(ui::get_content_region_available().x);
        const float plotWidth = ui::get_content_region_available().x;
        Rect plotRect{};

        // draw the plot
        {
            OSC_PERF("draw output plot");

            plot::push_style_var(plot::StyleVar::PlotPadding, {0.0f, 0.0f});
            plot::push_style_var(plot::StyleVar::PlotBorderSize, 0.0f);
            plot::push_style_var(plot::StyleVar::FitPadding, {0.0f, 1.0f});
            const auto flags = plot::PlotFlags::NoTitle | plot::PlotFlags::NoLegend | plot::PlotFlags::NoInputs | plot::PlotFlags::NoMenus | plot::PlotFlags::NoBoxSelect | plot::PlotFlags::NoFrame;

            if (plot::begin("##", {plotWidth, m_Height}, flags)) {
                plot::setup_axis(plot::Axis::X1, std::nullopt, plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus | plot::AxisFlags::AutoFit);
                plot::setup_axis(plot::Axis::Y1, std::nullopt, plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus | plot::AxisFlags::AutoFit);
                plot::push_style_color(plot::ColorVar::Line, Color::white().with_alpha(0.7f));
                plot::push_style_color(plot::ColorVar::PlotBackground, Color::clear());
                plot::plot_line("##", buf);
                plot::pop_style_color();
                plot::pop_style_color();

                plotRect = plot::get_plot_screen_rect();

                plot::end();
            }
            plot::pop_style_var();
            plot::pop_style_var();
            plot::pop_style_var();
        }

        // if the user right-clicks, draw a context menu
        TryDrawOutputContextMenuForLastItem(*m_API, sim, m_OutputExtractor);

        // (the rest): handle scrubber overlay
        OSC_PERF("draw output plot overlay");

        // figure out mapping between screen space and plot space

        SimulationClock::time_point simStartTime = sim.getSimulationReport(0).getTime();
        SimulationClock::time_point simEndTime = sim.getSimulationReport(nReports-1).getTime();
        SimulationClock::duration simTimeStep = (simEndTime-simStartTime)/nReports;
        SimulationClock::time_point simScrubTime = m_API->getSimulationScrubTime();

        float simScrubPct = static_cast<float>(static_cast<double>((simScrubTime - simStartTime)/(simEndTime - simStartTime)));

        ImDrawList* drawlist = ui::get_panel_draw_list();
        const ImU32 currentTimeLineColor = ui::to_ImU32(c_CurrentScubTimeColor);
        const ImU32 hoverTimeLineColor = ui::to_ImU32(c_HoveredScrubTimeColor);

        // draw a vertical Y line showing the current scrub time over the plots
        {
            float plotScrubLineX = plotRect.p1.x + simScrubPct*(dimensions_of(plotRect).x);
            Vec2 p1 = {plotScrubLineX, plotRect.p1.y};
            Vec2 p2 = {plotScrubLineX, plotRect.p2.y};
            drawlist->AddLine(p1, p2, currentTimeLineColor);
        }

        if (ui::is_item_hovered()) {
            Vec2 mp = ui::get_mouse_pos();
            Vec2 plotLoc = mp - plotRect.p1;
            float relLoc = plotLoc.x / dimensions_of(plotRect).x;
            SimulationClock::time_point timeLoc = simStartTime + relLoc*(simEndTime - simStartTime);

            // draw vertical line to show current X of their hover
            {
                Vec2 p1 = {mp.x, plotRect.p1.y};
                Vec2 p2 = {mp.x, plotRect.p2.y};
                drawlist->AddLine(p1, p2, hoverTimeLineColor);
            }

            // show a tooltip of X and Y
            {
                int step = static_cast<int>((timeLoc - simStartTime) / simTimeStep);
                if (0 <= step && static_cast<size_t>(step) < buf.size()) {
                    float y = buf[static_cast<size_t>(step)];

                    // ensure the tooltip doesn't occlude the line
                    ui::push_style_color(ImGuiCol_PopupBg, ui::get_style_color(ImGuiCol_PopupBg).with_alpha(0.5f));
                    ui::set_tooltip("(%.2fs, %.4f)", static_cast<float>(timeLoc.time_since_epoch().count()), y);
                    ui::pop_style_color();
                }
            }

            // if the user presses their left mouse while hovering over the plot,
            // change the current sim scrub time to match their press location
            if (ui::is_mouse_down(ImGuiMouseButton_Left)) {
                m_API->setSimulationScrubTime(timeLoc);
            }
        }
    }

    void drawStringOutputUI()
    {
        ISimulation& sim = m_API->updSimulation();
        const ptrdiff_t nReports = m_API->updSimulation().getNumReports();
        SimulationReport r = m_API->trySelectReportBasedOnScrubbing().value_or(sim.getSimulationReport(nReports - 1));

        ui::draw_text_centered(m_OutputExtractor.getValueString(*sim.getModel(), r));
        TryDrawOutputContextMenuForLastItem(*m_API, sim, m_OutputExtractor);
    }

    void drawVec2OutputUI()
    {
        OSC_ASSERT(m_OutputExtractor.getOutputType() == OutputExtractorDataType::Vec2);

        ISimulation& sim = m_API->updSimulation();

        const ptrdiff_t nReports = sim.getNumReports();
        if (nReports <= 0) {
            ui::draw_text("no data (yet)");
            return;
        }

        // collect output data from the `OutputExtractor`
        std::vector<Vec2> buf;
        {
            OSC_PERF("collect output data");
            std::vector<SimulationReport> reports = sim.getAllSimulationReports();
            buf = m_OutputExtractor.slurpValuesVec2(*sim.getModel(), reports);
        }

        // setup drawing area for drawing
        ui::set_next_item_width(ui::get_content_region_available().x);
        const float plotWidth = ui::get_content_region_available().x;
        Rect plotRect{};

        // draw the plot
        {
            OSC_PERF("draw output plot");

            plot::push_style_var(plot::StyleVar::PlotPadding, {0.0f, 0.0f});
            plot::push_style_var(plot::StyleVar::PlotBorderSize, 0.0f);
            plot::push_style_var(plot::StyleVar::FitPadding, {0.1f, 0.1f});
            plot::push_style_var(plot::StyleVar::AnnotationPadding, ui::get_style_panel_padding());
            const auto flags = plot::PlotFlags::NoTitle | plot::PlotFlags::NoLegend | plot::PlotFlags::NoMenus | plot::PlotFlags::NoBoxSelect | plot::PlotFlags::NoFrame;

            if (plot::begin("##", {plotWidth, m_Height}, flags)) {
                plot::setup_axis(plot::Axis::X1, std::nullopt, plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus | plot::AxisFlags::AutoFit);
                plot::setup_axis(plot::Axis::Y1, std::nullopt, plot::AxisFlags::NoDecorations | plot::AxisFlags::NoMenus | plot::AxisFlags::AutoFit);
                plot::push_style_color(plot::ColorVar::Line, Color::white().with_alpha(0.7f));
                plot::push_style_color(plot::ColorVar::PlotBackground, Color::clear());
                plot::plot_line("##", buf);
                plot::pop_style_color();
                plot::pop_style_color();

                plotRect = plot::get_plot_screen_rect();

                // overlays
                {
                    SimulationReport currentReport = m_API->trySelectReportBasedOnScrubbing().value_or(sim.getSimulationReport(nReports - 1));
                    Vec2d currentVal = m_OutputExtractor.getValueVec2(*sim.getModel(), currentReport);
                    // ensure the annotation doesn't occlude the line too heavily
                    auto annotationColor = ui::get_style_color(ImGuiCol_PopupBg).with_alpha(0.5f);
                    plot::draw_annotation(currentVal, annotationColor, {10.0f, 10.0f}, true, "(%f, %f)", currentVal.x, currentVal.y);
                    plot::drag_point(0, &currentVal, c_CurrentScubTimeColor, 4.0f, plot::DragToolFlags::NoInputs);
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
    OutputExtractor m_OutputExtractor;
    float m_Height;
};


osc::SimulationOutputPlot::SimulationOutputPlot(
    ISimulatorUIAPI* api,
    OutputExtractor outputExtractor,
    float height) :

    m_Impl{std::make_unique<Impl>(api, std::move(outputExtractor), height)}
{}

osc::SimulationOutputPlot::SimulationOutputPlot(SimulationOutputPlot&&) noexcept = default;
osc::SimulationOutputPlot& osc::SimulationOutputPlot::operator=(SimulationOutputPlot&&) noexcept = default;
osc::SimulationOutputPlot::~SimulationOutputPlot() noexcept = default;

void osc::SimulationOutputPlot::onDraw()
{
    m_Impl->onDraw();
}
