#include "SimulatorScreen.hpp"

#include "src/3D/Gl.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/UI/LogViewer.hpp"
#include "src/UI/MainMenu.hpp"
#include "src/UI/ComponentDetails.hpp"
#include "src/UI/ComponentHierarchy.hpp"
#include "src/UI/HelpMarker.hpp"
#include "src/Screens/ModelEditorScreen.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/App.hpp"
#include "src/Assertions.hpp"
#include "src/MainEditorState.hpp"
#include "src/Styling.hpp"
#include "src/os.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <imgui.h>
#include <nfd.h>

#include <limits>

using namespace osc;

namespace {
    // draw timescrubber slider
    void drawSimulationScrubber(
            osc::MainEditorState& st,
            UiSimulation& focusedSim,
            Report& focusedReport) {

        double t0 = 0.0f;
        double tf = focusedSim.simulation->simFinalTime().count();
        double treport = focusedReport.state.getTime();

        // draw the scrubber (slider)
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
        float v = static_cast<float>(treport);
        if (ImGui::SliderFloat("scrub", &v, static_cast<float>(t0), static_cast<float>(tf), "%.8f", ImGuiSliderFlags_AlwaysClamp)) {
            st.focusedSimulationScrubbingTime = v;
        }

        // draw hover-over tooltip for scrubber
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("Left-Click: Change simulation time being shown");
            ImGui::TextUnformatted("Ctrl-Click: Type in the simulation time being shown");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    // select a simulation report based on scubbing time
    [[nodiscard]] Report& selectReportBasedOnScrubbing(
            UiSimulation const& focused,
            float scrubTime) {

        auto& rr = focused.regularReports;

        // if there are no regular reports, use the spot report
        if (rr.empty()) {
            return *focused.spotReport;
        }

        // if the scrub time is negative (a senteniel), use the
        // spot report
        if (scrubTime < 0.0) {
            return *focused.spotReport;
        }

        // search through the regular reports for the first report that
        // finishes equal-to or after the scrub time
        auto startsAfterOrEqualToScrubTime = [&](std::unique_ptr<Report> const& report) {
            return report->state.getTime() >= scrubTime;
        };

        auto it = std::find_if(rr.begin(), rr.end(), startsAfterOrEqualToScrubTime);

        // if no such report is found, use the spot report
        if (it == rr.end()) {
            return *focused.spotReport;
        }

        return **it;
    }
}

// simulator screeen (private) state
struct osc::SimulatorScreen::Impl final {

    // top-level state: shared between editor+sim screens
    std::shared_ptr<MainEditorState> mes;

    // scratch space for plots
    std::vector<float> plotscratch;

    // ui component state
    LogViewer logViewerState;
    MainMenuFileTab mmFileTab;
    MainMenuWindowTab mmWindowTab;
    MainMenuAboutTab mmAboutTab;

    Impl(std::shared_ptr<MainEditorState> _mes) : mes{std::move(_mes)} {

        // lazily init at least one viewer
        if (!mes->viewers.front()) {
            mes->viewers.front() = std::make_unique<UiModelViewer>();
        }
    }
};

// private Impl-related functions
namespace {

    // start a new simulation from whatever the user's currently editing
    void actionStartSimulationFromEditedModel(osc::MainEditorState& impl) {
        impl.startSimulatingEditedModel();
    }

    // pop all updates from all active simulations
    void popAllSimulatorUpdates(osc::MainEditorState& impl) {
        for (auto& simulation : impl.simulations) {
            // pop regular reports
            {
                auto& rr = simulation->regularReports;
                int popped = simulation->simulation->popRegularReports(rr);

                for (size_t i = rr.size() - static_cast<size_t>(popped); i < rr.size(); ++i) {
                    simulation->model->realizeReport(rr[i]->state);
                }
            }

            // pop latest spot report
            std::unique_ptr<Report> newSpotReport = simulation->simulation->tryPopLatestReport();
            if (newSpotReport) {
                simulation->spotReport = std::move(newSpotReport);
                simulation->model->realizeReport(simulation->spotReport->state);
            }
        }
    }

    // draw details of one simulation
    void drawSimulationProgressBarEtc(SimulatorScreen::Impl& impl, int i) {
        MainEditorState& st = *impl.mes;

        if (!(0 <= i && i < static_cast<int>(st.simulations.size()))) {
            ImGui::TextUnformatted("(invalid simulation index)");
            return;
        }
        UiSimulation& simulation = *st.simulations[i];

        ImGui::PushID(static_cast<int>(i));

        float progress = simulation.simulation->progress();
        ImVec4 baseColor = progress >= 1.0f ? ImVec4{0.0f, 0.7f, 0.0f, 0.5f} : ImVec4{0.7f, 0.7f, 0.0f, 0.5f};
        if (static_cast<int>(i) == st.focusedSimulation) {
            baseColor.w = 1.0f;
        }

        bool shouldErase = false;

        if (ImGui::Button("x")) {
            shouldErase = true;
        }

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, baseColor);
        ImGui::ProgressBar(progress);
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered()) {
            if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
                shouldErase = true;
            }

            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);
            ImGui::TextUnformatted(simulation.model->getName().c_str());
            ImGui::Dummy(ImVec2{0.0f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_SLIGHTLY_GREYED_RGBA);
            ImGui::Text("Wall time (sec): %.1f", simulation.simulation->wallDuration().count());
            ImGui::Text("Sim time (sec): %.1f", simulation.simulation->simCurrentTime().count());
            ImGui::Text("Sim final time (sec): %.1f", simulation.simulation->simFinalTime().count());
            ImGui::Dummy(ImVec2{0.0f, 1.0f});
            ImGui::TextUnformatted("Left-click: Select this simulation");
            ImGui::TextUnformatted("Delete: cancel this simulation");
            ImGui::PopStyleColor();
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            st.focusedSimulation = static_cast<int>(i);
        }

        if (ImGui::BeginPopupContextItem("simcontextmenu")) {
            st.focusedSimulation = static_cast<int>(i);

            if (ImGui::MenuItem("edit model")) {
                auto copy = std::make_unique<OpenSim::Model>(*simulation.model);
                st.setModel(std::move(copy));
                App::cur().requestTransition<ModelEditorScreen>(impl.mes);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);
                ImGui::TextUnformatted("Make the model initially used in this simulation into the model being edited in the editor");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            ImGui::EndPopup();
        }

        if (shouldErase) {
            st.simulations.erase(st.simulations.begin() + i);
            if (static_cast<int>(i) <= st.focusedSimulation) {
                --st.focusedSimulation;
            }
        }

        ImGui::PopID();
    }

    // draw top-level "Simulation" tab that lists all simulations
    void drawSimulationTab(SimulatorScreen::Impl& impl) {
         osc::MainEditorState& st = *impl.mes;

        // draw scrubber for currently-selected sim
        ImGui::TextUnformatted("Scrubber:");
        ImGui::Separator();
        ImGui::Dummy(ImVec2{0.0f, 0.3f});
        UiSimulation* sim = st.getFocusedSim();
        if (sim) {
            Report& report = selectReportBasedOnScrubbing(*sim, st.focusedSimulationScrubbingTime);
            drawSimulationScrubber(st, *sim, report);
        } else {
            ImGui::TextDisabled("(no simulation selected)");
        }

        // draw simulations list
        ImGui::Dummy(ImVec2{0.0f, 1.0f});
        ImGui::TextUnformatted("Simulations:");
        ImGui::Separator();
        ImGui::Dummy(ImVec2{0.0f, 0.3f});
        for (size_t i = 0; i < st.simulations.size(); ++i) {
            drawSimulationProgressBarEtc(impl, static_cast<int>(i));
        }
    }

// draw a plot for an integrator stat
#define OSC_DRAW_SIMSTAT_PLOT(statname) \
    {                                                                                                                  \
        scratch.clear();                                                                                               \
        for (auto const& report : focused.regularReports) {                                                            \
            auto const& stats = report->stats;                                                                         \
            scratch.push_back(static_cast<float>(stats.statname));                                                     \
        }                                                                                                              \
        ImGui::TextUnformatted(#statname); \
        ImGui::SameLine();  \
        DrawHelpMarker(SimStats::g_##statname##Desc);  \
        ImGui::NextColumn();  \
        ImGui::PushID(imgui_id++);  \
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());  \
        ImGui::PlotLines("##"#statname, scratch.data(), static_cast<int>(scratch.size()), 0, nullptr, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), ImVec2(0.0f, .0f));                                 \
        ImGui::PopID(); \
        ImGui::NextColumn(); \
    }

    // draw top-level "simulation stats" tab that shows integrator stats etc. for
    // the focused simulation
    void drawSimulationStatsTab(osc::SimulatorScreen::Impl& impl) {
        UiSimulation const* maybeFocused = impl.mes->getFocusedSim();

        if (!maybeFocused) {
            ImGui::TextDisabled("(no simulation selected)");
            return;
        }
        UiSimulation const& focused = *maybeFocused;

        ImGui::Dummy(ImVec2{0.0f, 1.0f});
        ImGui::TextUnformatted("parameters:");
        ImGui::SameLine();
        DrawHelpMarker("The parameters used when this simulation was launched. These must be set *before* running the simulation");
        ImGui::Separator();
        ImGui::Dummy(ImVec2{0.0f, 2.0f});

        // draw simulation parameters
        {
            FdParams const& p = focused.simulation->params();

            ImGui::Columns(2);

            ImGui::TextUnformatted(p.g_FinalTimeTitle);
            ImGui::SameLine();
            DrawHelpMarker(p.g_FinalTimeDesc);
            ImGui::NextColumn();
            ImGui::Text("%f", p.FinalTime.count());
            ImGui::NextColumn();

            ImGui::TextUnformatted(p.g_ThrottleToWallTimeTitle);
            ImGui::SameLine();
            DrawHelpMarker(p.g_ThrottleToWallTimeDesc);
            ImGui::NextColumn();
            ImGui::TextUnformatted(p.ThrottleToWallTime ? "true" : "false");
            ImGui::NextColumn();

            ImGui::TextUnformatted(p.g_IntegratorMethodUsedTitle);
            ImGui::SameLine();
            DrawHelpMarker(p.g_IntegratorMethodUsedDesc);
            ImGui::NextColumn();
            ImGui::TextUnformatted(g_IntegratorMethodNames[p.IntegratorMethodUsed]);
            ImGui::NextColumn();

            ImGui::TextUnformatted(p.g_ReportingIntervalTitle);
            ImGui::SameLine();
            DrawHelpMarker(p.g_ReportingIntervalDesc);
            ImGui::NextColumn();
            ImGui::Text("%f", p.ReportingInterval.count());
            ImGui::NextColumn();

            ImGui::TextUnformatted(p.g_IntegratorStepLimitTitle);
            ImGui::SameLine();
            DrawHelpMarker(p.g_IntegratorStepLimitDesc);
            ImGui::NextColumn();
            ImGui::Text("%i", p.IntegratorStepLimit);
            ImGui::NextColumn();

            ImGui::TextUnformatted(p.g_IntegratorMinimumStepSizeTitle);
            ImGui::SameLine();
            DrawHelpMarker(p.g_IntegratorMinimumStepSizeDesc);
            ImGui::NextColumn();
            ImGui::Text("%f", p.IntegratorMinimumStepSize.count());
            ImGui::NextColumn();

            ImGui::TextUnformatted(p.g_IntegratorMaximumStepSizeTitle);
            ImGui::SameLine();
            DrawHelpMarker(p.g_IntegratorMaximumStepSizeDesc);
            ImGui::NextColumn();
            ImGui::Text("%f", p.IntegratorMaximumStepSize.count());
            ImGui::NextColumn();

            ImGui::TextUnformatted(p.g_IntegratorAccuracyTitle);
            ImGui::SameLine();
            DrawHelpMarker(p.g_IntegratorAccuracyDesc);
            ImGui::NextColumn();
            ImGui::Text("%f", p.IntegratorAccuracy);
            ImGui::NextColumn();

            ImGui::TextUnformatted(p.g_UpdateLatestStateOnEveryStepTitle);
            ImGui::SameLine();
            DrawHelpMarker(p.g_UpdateLatestStateOnEveryStepDesc);
            ImGui::NextColumn();
            ImGui::TextUnformatted(p.UpdateLatestStateOnEveryStep ? "true" : "false");
            ImGui::NextColumn();

            ImGui::Columns();
        }

        ImGui::Dummy(ImVec2{0.0f, 10.0f});
        ImGui::TextUnformatted("plots:");
        ImGui::SameLine();
        DrawHelpMarker("These plots are collected from the underlying simulation engine as the simulation runs. The data is heavily affected by the model's structure, choice of integrator, and simulation settings");
        ImGui::Separator();
        ImGui::Dummy(ImVec2{0.0f, 2.0f});

        // draw simulation stat plots
        {
            std::vector<float>& scratch = impl.plotscratch;
            int imgui_id = 0;

            ImGui::Columns(2);
            OSC_DRAW_SIMSTAT_PLOT(AccuracyInUse);
            OSC_DRAW_SIMSTAT_PLOT(NumConvergenceTestFailures);
            OSC_DRAW_SIMSTAT_PLOT(NumConvergentIterations);
            OSC_DRAW_SIMSTAT_PLOT(NumDivergentIterations);
            OSC_DRAW_SIMSTAT_PLOT(NumErrorTestFailures);
            OSC_DRAW_SIMSTAT_PLOT(NumIterations);
            OSC_DRAW_SIMSTAT_PLOT(NumProjectionFailures);
            OSC_DRAW_SIMSTAT_PLOT(NumQProjections);
            OSC_DRAW_SIMSTAT_PLOT(NumQProjectionFailures);
            OSC_DRAW_SIMSTAT_PLOT(NumRealizations);
            OSC_DRAW_SIMSTAT_PLOT(NumRealizationFailures);
            OSC_DRAW_SIMSTAT_PLOT(NumStepsAttempted);
            OSC_DRAW_SIMSTAT_PLOT(NumStepsTaken);
            OSC_DRAW_SIMSTAT_PLOT(NumUProjections);
            OSC_DRAW_SIMSTAT_PLOT(NumUProjectionFailures);
            OSC_DRAW_SIMSTAT_PLOT(PredictedNextStepSize);
            ImGui::Columns();
        }
    }

    // action to take when user presses a key
    bool simscreenOnKeydown(osc::SimulatorScreen::Impl& impl, SDL_KeyboardEvent const& e) {
        if (e.keysym.mod & KMOD_CTRL) {
            // Ctrl

            switch (e.keysym.sym) {
            case SDLK_e:
                // Ctrl + e
                App::cur().requestTransition<ModelEditorScreen>(std::move(impl.mes));
                return true;
            }
        }
        return false;
    }

    // action to take when a generic event occurs
    bool simscreenOnEvent(osc::SimulatorScreen::Impl& impl, SDL_Event const& e) {
        if (e.type == SDL_KEYDOWN) {
            if (simscreenOnKeydown(impl, e.key)) {
                return true;
            }
        }
        return false;
    }

    // draw output plots for the currently-focused sim
    void drawOutputPlots(
            osc::SimulatorScreen::Impl& impl,
            UiSimulation const& focusedSim,
            Report const& focusedReport,
            OpenSim::Component const& selected) {

        int imguiID = 0;

        ImGui::Columns(2);
        for (auto const& ptr : selected.getOutputs()) {
            OpenSim::AbstractOutput const& ao = *ptr.second;

            ImGui::TextUnformatted(ao.getName().c_str());
            ImGui::NextColumn();

            OpenSim::Output<double> const* odPtr =
                dynamic_cast<OpenSim::Output<double> const*>(&ao);

            if (!odPtr) {
                ImGui::TextUnformatted(ao.getValueAsString(focusedReport.state).c_str());
                ImGui::NextColumn();
                continue;  // unplottable
            }

            OpenSim::Output<double> const& od = *odPtr;

            size_t npoints = focusedSim.regularReports.size();
            impl.plotscratch.resize(npoints);
            size_t i = 0;
            for (auto const& report : focusedSim.regularReports) {
                double v = od.getValue(report->state);
                impl.plotscratch[i++] = static_cast<float>(v);
            }

            ImGui::PushID(imguiID++);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
            ImGui::PlotLines("##nolabel", impl.plotscratch.data(), static_cast<int>(impl.plotscratch.size()), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2{0.0f, 20.0f});

            if (ImGui::BeginPopupContextItem(od.getName().c_str())) {
                if (ImGui::MenuItem("Add to outputs watch")) {
                    impl.mes->desiredOutputs.emplace_back(selected, od);
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
            ImGui::NextColumn();
        }
        ImGui::Columns();
    }

    // draw "selection" tab, which shows details of current selection
    void drawSelectionTab(osc::SimulatorScreen::Impl& impl) {

        UiSimulation const* maybeSim = impl.mes->getFocusedSim();

        if (!maybeSim) {
            ImGui::TextDisabled("(no simulation selected)");
            return;
        }
        UiSimulation const& sim = *maybeSim;

        if (!sim.selected) {
            ImGui::TextDisabled("(no component selected)");
            return;
        }

        Report const& report = selectReportBasedOnScrubbing(sim, impl.mes->focusedSimulationScrubbingTime);

        ComponentDetails{}.draw(report.state, sim.selected);

        if (ImGui::CollapsingHeader("outputs")) {
            drawOutputPlots(impl, sim, report, *sim.selected);
        }
    }

    // export a timeseries to a CSV file and return the filepath
    std::string exportTimeseriesToCSV(
            float const* ts,    // times
            float const* vs,    // values @ each time in times
            size_t n,           // number of datapoints
            char const* vname   // name of values (header)
            ) {

        nfdchar_t* outpath = nullptr;
        nfdresult_t result = NFD_SaveDialog("csv", nullptr, &outpath);
        OSC_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

        if (result != NFD_OKAY) {
            return "";  // user cancelled out
        }

        std::ofstream fout{outpath};

        if (!fout) {
            log::error("%s: error opening file for writing", outpath);
            return "";  // error opening output file for writing
        }

        fout << "t," << vname << '\n';
        for (size_t i = 0; i < n; ++i) {
            fout << ts[i] << ',' << vs[i] << '\n';
        }

        if (!fout) {
            log::error("%s: error encountered while writing CSV data to file", outpath);
            return "";  // error writing
        }

        log::info("%: successfully wrote CSV data to output file", outpath);

        return std::string{outpath};
    }

    // export all plotted outputs to a single CSV file and return the filepath
    std::string exportAllPlottedOutputsToCSV(
            SimulatorScreen::Impl const& impl,
            UiSimulation const& sim) {

        // try prompt user for save location
        nfdchar_t* outpath = nullptr;
        nfdresult_t result = NFD_SaveDialog("csv", nullptr, &outpath);
        OSC_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

        if (result != NFD_OKAY) {
            return "";  // user cancelled out
        }

        // try to open the output file
        std::ofstream fout{outpath};
        if (!fout) {
            log::error("%s: error opening file for writing", outpath);
            return "";  // error opening output file for writing
        }

        struct PlottableOutput final {
            OpenSim::Component const& component;
            OpenSim::AbstractOutput const& output;
            extrator_fn_t extractor;

            PlottableOutput(
                OpenSim::Component const& _component,
                OpenSim::AbstractOutput const& _output,
                extrator_fn_t _extractor) :

                component{_component},
                output{_output},
                extractor{_extractor} {
            }
        };

        // collect plottable outputs
        std::vector<PlottableOutput> plottableOutputs;
        for (DesiredOutput const& de : impl.mes->desiredOutputs) {
            if (!de.extractorFunc) {
                continue;  // no extractor function
            }

            OpenSim::Component const* cp = nullptr;
            try {
                cp = &sim.model->getComponent(de.absoluteComponentPath);
            } catch (...) {
                // OpenSim, innit
                //
                // avoid OpenSim::Component::findComponent like the plague
                // because it's written by someone who seems to be pathalogically
                // allergic to performance
            }

            if (!cp) {
                continue;  // the component doesn't exist
            }

            OpenSim::Component const& c = *cp;

            OpenSim::AbstractOutput const* aop = nullptr;
            try {
                aop = &c.getOutput(de.outputName);
            } catch (...) {
                // OpenSim, innit
            }

            if (!aop) {
                continue;  // the output doesn't exist on the component
            }

            size_t typehash = typeid(*aop).hash_code();

            if (typehash != de.outputTypeHashcode) {
                continue;  // the output is there, but now has a different type
            }

            plottableOutputs.emplace_back(c, *aop, de.extractorFunc);
        }

        // write header line
        fout << "time";
        for (PlottableOutput const& po : plottableOutputs) {
            fout << ',' << po.component.getName() << '_' << po.output.getName();
        }
        fout << '\n';

        // write data lines
        for (auto const& report : sim.regularReports) {
            SimTK::State const& stkst = report->state;

            // write time
            fout << stkst.getTime();
            for (PlottableOutput const& po : plottableOutputs) {
                fout << ',' << po.extractor(po.output, stkst);
            }
            fout << '\n';
        }

        // check writing was successful
        //
        // this is just a sanity check: it will be written regardless
        if (!fout) {
            log::warn("%s: encountered error while writing output data: some of the data may have been written, but maybe not all of it", outpath);
        }

        return std::string{outpath};
    }

    // draw "outputs" tab, which shows user-selected simulation outputs
    void drawOutputsTab(osc::SimulatorScreen::Impl& impl) {

        UiSimulation const* maybeSim = impl.mes->getFocusedSim();
        if (!maybeSim) {
            ImGui::TextDisabled("(no simulation selected)");
            return;
        }
        UiSimulation const& sim = *maybeSim;

        Report const& report = selectReportBasedOnScrubbing(sim, impl.mes->focusedSimulationScrubbingTime);

        int imguiID = 0;
        MainEditorState& st = *impl.mes;

        if (st.desiredOutputs.empty()) {
            ImGui::TextUnformatted("No outputs being plotted: right-click them in the model editor");
            return;
        }

        if (ImGui::Button("Save all to CSV")) {
            exportAllPlottedOutputsToCSV(impl, sim);
        }

        ImGui::SameLine();

        if (ImGui::Button("Save all to CSV & Open")) {
            std::string path = exportAllPlottedOutputsToCSV(impl, sim);
            if (!path.empty()) {
                OpenPathInOSDefaultApplication(path);
            }
        }

        ImGui::Dummy(ImVec2{0.0f, 5.0f});

        ImGui::Columns(2);
        for (DesiredOutput const& de : st.desiredOutputs) {
            ImGui::Text("%s[%s]", de.absoluteComponentPath.c_str(), de.outputName.c_str());
            ImGui::NextColumn();

            OpenSim::Component const* cp = nullptr;
            try {
                cp = &sim.model->getComponent(de.absoluteComponentPath);
            } catch (...) {
                // OpenSim, innit
                //
                // avoid OpenSim::Component::findComponent like the plague
                // because it's written by someone who seems to be pathalogically
                // allergic to performance
            }

            if (!cp) {
                ImGui::TextUnformatted("component not found");
                ImGui::NextColumn();
                continue;
            }

            OpenSim::Component const& c = *cp;

            OpenSim::AbstractOutput const* aop = nullptr;
            try {
                aop = &c.getOutput(de.outputName);
            } catch (...) {
                // OpenSim, innit
            }

            if (!aop) {
                ImGui::TextUnformatted("output not found");
                ImGui::NextColumn();
                continue;
            }

            OpenSim::AbstractOutput const& ao = *aop;
            size_t typehash = typeid(ao).hash_code();

            if (typehash != de.outputTypeHashcode) {
                ImGui::TextUnformatted("output type changed");
                ImGui::NextColumn();
                continue;
            }

            if (!de.extractorFunc) {
                // no extractor function, so unplottable
                ImGui::TextUnformatted(ao.getValueAsString(report.state).c_str());
                ImGui::NextColumn();
                continue;
            }

            // else: it's a plottable output

            size_t npoints = sim.regularReports.size();
            impl.plotscratch.resize(npoints);
            size_t i = 0;
            for (auto const& r : sim.regularReports) {
                double v = de.extractorFunc(ao, r->state);
                impl.plotscratch[i++] = static_cast<float>(v);
            }

            ImGui::PushID(imguiID++);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
            ImGui::PlotLines("##nolabel", impl.plotscratch.data(), static_cast<int>(impl.plotscratch.size()));
            if (ImGui::BeginPopupContextItem("outputplotscontextmenu")) {
                if (ImGui::MenuItem("Save as CSV")) {
                    std::vector<float> ts;
                    ts.reserve(sim.regularReports.size());
                    for (auto const& r : sim.regularReports) {
                        ts.push_back(static_cast<float>(r->state.getTime()));
                    }
                    OSC_ASSERT_ALWAYS(ts.size() == impl.plotscratch.size());
                    exportTimeseriesToCSV(ts.data(), impl.plotscratch.data(), ts.size(), ao.getName().c_str());
                }

                if (ImGui::MenuItem("Save as CSV & Open")) {
                    std::vector<float> ts;
                    ts.reserve(sim.regularReports.size());
                    for (auto const& r : sim.regularReports) {
                        ts.push_back(static_cast<float>(r->state.getTime()));
                    }
                    OSC_ASSERT_ALWAYS(ts.size() == impl.plotscratch.size());
                    std::string outpath = exportTimeseriesToCSV(ts.data(), impl.plotscratch.data(), ts.size(), ao.getName().c_str());

                    if (!outpath.empty()) {
                        OpenPathInOSDefaultApplication(outpath);
                    }
                }

                ImGui::EndPopup();
            }
            ImGui::PopID();
            ImGui::NextColumn();
        }
        ImGui::Columns();
    }

    // draw "hierarchy" tab, which shows the tree hierarchy structure for
    // the currently-focused sim
    void drawHierarchyTab(SimulatorScreen::Impl& impl) {
        UiSimulation* maybeSim = impl.mes->getFocusedSim();

        if (!maybeSim) {
            ImGui::TextDisabled("(no simulation selected)");
            return;
        }
        UiSimulation& sim = *maybeSim;

        auto resp = ComponentHierarchy{}.draw(sim.model.get(), sim.selected, sim.hovered);

        if (resp.type == ComponentHierarchy::SelectionChanged) {
            sim.selected = const_cast<OpenSim::Component*>(resp.ptr);
        } else if (resp.type == ComponentHierarchy::HoverChanged) {
            sim.hovered = const_cast<OpenSim::Component*>(resp.ptr);
        }
    }

    class RenderableSim final : public RenderableScene {
        UiSimulation& m_Sim;
        Report const& m_Report;
        std::vector<LabelledSceneElement> m_Decorations;
        BVH m_SceneBVH;
        float m_FixupScaleFactor;

    public:
        RenderableSim(UiSimulation& sim, Report const& report) :
            m_Sim{sim},
            m_Report{report},
            m_Decorations{},
            m_SceneBVH{},
            m_FixupScaleFactor{1.0f} // todo
        {
            generateDecorations(*m_Sim.model, m_Report.state, m_FixupScaleFactor, m_Decorations);
            updateBVH(m_Decorations, m_SceneBVH);
        }

        ~RenderableSim() noexcept override = default;

        nonstd::span<LabelledSceneElement const> getSceneDecorations() const override {
            return m_Decorations;
        }

        BVH const& getSceneBVH() const override {
            return m_SceneBVH;
        }

        float getFixupScaleFactor() const override {
            return m_FixupScaleFactor;
        }

        OpenSim::Component const* getSelected() const override {
            return m_Sim.selected;
        }

        OpenSim::Component const* getHovered() const override {
            return m_Sim.hovered;
        }

        OpenSim::Component const* getIsolated() const override {
            return nullptr;
        }
    };

    // draw a 3D model viewer
    void draw3DViewer(
            UiSimulation& sim,
            Report const& report,
            UiModelViewer& viewer,
            char const* name) {

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        OSC_SCOPE_GUARD({ ImGui::PopStyleVar(); });
        if (ImGui::Begin(name, nullptr, ImGuiWindowFlags_MenuBar)) {
            RenderableSim rs{sim, report};
            auto resp = viewer.draw(rs);

            if (resp.isLeftClicked && resp.hovertestResult) {
                sim.selected = const_cast<OpenSim::Component*>(resp.hovertestResult);
            }
            if (resp.isMousedOver && resp.hovertestResult != sim.hovered) {
                sim.hovered = const_cast<OpenSim::Component*>(resp.hovertestResult);
            }
        }
        ImGui::End();
    }

    // draw all active 3D viewers
    //
    // the user can (de)activate 3D viewers in the "Window" tab
    void drawAll3DViewers(SimulatorScreen::Impl& impl) {
        UiSimulation* maybeSim = impl.mes->getFocusedSim();

        if (!maybeSim) {
            if (ImGui::Begin("render")) {
                ImGui::TextDisabled("(no simulation selected)");
            }
            ImGui::End();
            return;
        }

        UiSimulation& sim = *maybeSim;
        Report const& report = selectReportBasedOnScrubbing(sim, impl.mes->focusedSimulationScrubbingTime);
        MainEditorState& st = *impl.mes;

        for (size_t i = 0; i < st.viewers.size(); ++i) {
            auto& maybeViewer = st.viewers[i];

            if (!maybeViewer) {
                continue;
            }

            UiModelViewer& viewer = *maybeViewer;

            char buf[64];
            std::snprintf(buf, sizeof(buf), "viewer%zu", i);

            draw3DViewer(sim, report, viewer, buf);
        }
    }

    // draw the simulator screen
    void simscreenDraw(osc::SimulatorScreen::Impl& impl) {

        // draw main menu
        if (ImGui::BeginMainMenuBar()) {
            impl.mmFileTab.draw(impl.mes);
            impl.mmWindowTab.draw(*impl.mes);
            impl.mmAboutTab.draw();

            ImGui::Dummy(ImVec2{5.0f, 0.0f});

            if (ImGui::Button(ICON_FA_CUBE " Switch to editor (Ctrl+E)")) {

                // request the transition then exit this drawcall ASAP
                App::cur().requestTransition<ModelEditorScreen>(std::move(impl.mes));
                ImGui::EndMainMenuBar();
                return;
            }

            ImGui::EndMainMenuBar();
        }

        MainEditorState& st = *impl.mes;

        // edge-case: there are no simulations available, so
        // show a "you need to run something, fool" dialog
        if (!st.hasSimulations()) {
            if (ImGui::Begin("Warning")) {
                ImGui::TextUnformatted("No simulations are currently running");
                if (ImGui::Button("Run new simulation")) {
                    actionStartSimulationFromEditedModel(st);
                }
            }
            ImGui::End();
            return;
        }

        // draw simulations tab
        if (st.showing.simulations) {
            if (ImGui::Begin("Simulations", &st.showing.simulations)) {
                drawSimulationTab(impl);
            }
            ImGui::End();
        }

        // draw 3d viewers
        {
            drawAll3DViewers(impl);
        }

        // draw hierarchy tab
        if (st.showing.hierarchy) {
            if (ImGui::Begin("Hierarchy", &st.showing.hierarchy)) {
                drawHierarchyTab(impl);
            }
            ImGui::End();
        }

        // draw selection tab
        if (st.showing.selectionDetails) {
            if (ImGui::Begin("Selection", &st.showing.selectionDetails)) {
                drawSelectionTab(impl);
            }
            ImGui::End();
        }

        // draw outputs tab
        if (st.showing.outputs) {
            if (ImGui::Begin("Outputs", &st.showing.outputs)) {
                drawOutputsTab(impl);
            }
            ImGui::End();
        }

        // draw simulation stats tab
        if (st.showing.simulationStats) {
            if (ImGui::Begin("Simulation Details", &st.showing.simulationStats)) {
                drawSimulationStatsTab(impl);
            }
            ImGui::End();
        }

        // draw log tab
        if (st.showing.log) {
            if (ImGui::Begin("Log", &st.showing.log, ImGuiWindowFlags_MenuBar)) {
                impl.logViewerState.draw();
            }
            ImGui::End();
        }
    }
}

// Simulator_screen: public impl.

osc::SimulatorScreen::SimulatorScreen(std::shared_ptr<MainEditorState> mes) :
    m_Impl{new Impl{std::move(mes)}} {
}

osc::SimulatorScreen::~SimulatorScreen() noexcept = default;

void osc::SimulatorScreen::onMount() {
    osc::ImGuiInit();
}

void osc::SimulatorScreen::onUnmount() {
    osc::ImGuiShutdown();
}

void osc::SimulatorScreen::onEvent(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    ::simscreenOnEvent(*m_Impl, e);
}

void osc::SimulatorScreen::tick(float) {
    popAllSimulatorUpdates(*m_Impl->mes);
}

void osc::SimulatorScreen::draw() {
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    osc::ImGuiNewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);
    ::simscreenDraw(*m_Impl);
    osc::ImGuiRender();
}
