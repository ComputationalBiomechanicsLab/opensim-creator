#include "simulator_screen.hpp"

#include "src/main_editor_state.hpp"
#include "src/opensim_bindings/fd_simulation.hpp"
#include "src/ui/log_viewer.hpp"
#include "src/ui/model_viewer.hpp"
#include "src/ui/main_menu.hpp"
#include "src/ui/component_details.hpp"
#include "src/ui/component_hierarchy.hpp"
#include "src/screens/model_editor_screen.hpp"
#include "src/assertions.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <imgui.h>

#include <limits>

using namespace osc;

struct osc::Simulator_screen::Impl final {
    // top-level state: shared between edit+sim screens
    std::shared_ptr<Main_editor_state> st;

    // scratch space for plots
    std::vector<float> plotscratch;

    // ui component state
    osc::ui::log_viewer::State log_viewer_st;
    osc::ui::main_menu::file_tab::State mm_filetab_st;
    
    Model_viewer_widget viewer{ModelViewerWidgetFlags_Default | ModelViewerWidgetFlags_DrawFrames};

    Impl(std::shared_ptr<Main_editor_state> _st) : st {std::move(_st)} {
    }
};

static void action_start_simulation(osc::Main_editor_state& impl) {
    impl.simulations.emplace_back(new Ui_simulation{impl.model(), impl.state(), impl.sim_params});
}

static void pop_all_simulator_updates(osc::Main_editor_state& impl) {
    for (auto& simulation : impl.simulations) {
        // pop regular reports
        {
            auto& rr = simulation->regular_reports;
            int popped = simulation->simulation.pop_regular_reports(rr);

            for (size_t i = rr.size() - static_cast<size_t>(popped); i < rr.size(); ++i) {
                simulation->model->realizeReport(rr[i]->state);
            }
        }

        // pop latest spot report
        std::unique_ptr<fd::Report> new_spot_report = simulation->simulation.try_pop_latest_report();
        if (new_spot_report) {
            simulation->spot_report = std::move(new_spot_report);
            simulation->model->realizeReport(simulation->spot_report->state);
        }
    }
}

static void draw_simulation(osc::Main_editor_state& impl, int i) {
    Ui_simulation& simulation = *impl.simulations[i];

    ImGui::PushID(static_cast<int>(i));

    float progress = simulation.simulation.progress();
    ImVec4 base_color = progress >= 1.0f ? ImVec4{0.0f, 0.7f, 0.0f, 0.5f} : ImVec4{0.7f, 0.7f, 0.0f, 0.5f};
    if (static_cast<int>(i) == impl.focused_simulation) {
        base_color.w = 1.0f;
    }

    bool should_erase = false;

    if (ImGui::Button("x")) {
        should_erase = true;
    }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, base_color);
    ImGui::ProgressBar(progress);
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered()) {
        if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
            should_erase = true;
        }

        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);
        ImGui::TextUnformatted(simulation.model->getName().c_str());
        ImGui::Dummy(ImVec2{0.0f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.7f, 0.7f, 0.7f, 1.0f});
        ImGui::Text("Wall time (sec): %.1f", simulation.simulation.wall_duration().count());
        ImGui::Text("Sim time (sec): %.1f", simulation.simulation.sim_current_time().count());
        ImGui::Text("Sim final time (sec): %.1f", simulation.simulation.sim_final_time().count());
        ImGui::Dummy(ImVec2{0.0f, 1.0f});
        ImGui::TextUnformatted("Left-click: Select this simulation");
        ImGui::TextUnformatted("Delete: cancel this simulation");
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        OSC_ASSERT(i <= std::numeric_limits<int>::max());
        impl.focused_simulation = static_cast<int>(i);
    }

    if (should_erase) {
        impl.simulations.erase(impl.simulations.begin() + i);
        if (static_cast<int>(i) <= impl.focused_simulation) {
            --impl.focused_simulation;
        }
    }

    ImGui::PopID();
}

static void draw_simulation_tab(osc::Main_editor_state& impl) {
    for (size_t i = 0; i < impl.simulations.size(); ++i) {
        draw_simulation(impl, static_cast<int>(i));
    }
}

#define OSC_MAKE_SIMSTAT_PLOT(statname) \
    {                                                                                                                  \
        scratch.clear();                                                                                               \
        for (auto const& report : focused.regular_reports) {                                                           \
            auto const& stats = report->stats;                                                                         \
            scratch.push_back(static_cast<float>(stats.statname));                                                     \
        }                                                                                                              \
        ImGui::PlotLines(#statname, scratch.data(), static_cast<int>(scratch.size()), 0, nullptr, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), ImVec2(0.0f, 30.0f));                                 \
    }

static void draw_simulation_stats_tab(osc::Simulator_screen::Impl& impl, Ui_simulation& focused) {

    std::vector<float>& scratch = impl.plotscratch;

    OSC_MAKE_SIMSTAT_PLOT(accuracyInUse);
    OSC_MAKE_SIMSTAT_PLOT(predictedNextStepSize);
    OSC_MAKE_SIMSTAT_PLOT(numStepsAttempted);
    OSC_MAKE_SIMSTAT_PLOT(numStepsTaken);
    OSC_MAKE_SIMSTAT_PLOT(numRealizations);
    OSC_MAKE_SIMSTAT_PLOT(numQProjections);
    OSC_MAKE_SIMSTAT_PLOT(numUProjections);
    OSC_MAKE_SIMSTAT_PLOT(numErrorTestFailures);
    OSC_MAKE_SIMSTAT_PLOT(numConvergenceTestFailures);
    OSC_MAKE_SIMSTAT_PLOT(numRealizationFailures);
    OSC_MAKE_SIMSTAT_PLOT(numQProjectionFailures);
    OSC_MAKE_SIMSTAT_PLOT(numUProjectionFailures);
    OSC_MAKE_SIMSTAT_PLOT(numProjectionFailures);
    OSC_MAKE_SIMSTAT_PLOT(numConvergentIterations);
    OSC_MAKE_SIMSTAT_PLOT(numDivergentIterations);
    OSC_MAKE_SIMSTAT_PLOT(numIterations);
}

static bool on_keydown(osc::Simulator_screen::Impl& impl, SDL_KeyboardEvent const& e) {
    if (e.keysym.mod & KMOD_CTRL) {
        // Ctrl

        switch (e.keysym.sym) {
        case SDLK_e:
            // Ctrl + e
            Application::current().request_screen_transition<Model_editor_screen>(std::move(impl.st));
            return true;
        }
    }
    return false;
}

static bool on_event(osc::Simulator_screen::Impl& impl, SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN) {
        if (on_keydown(impl, e.key)) {
            return true;
        }
    }

    if (impl.viewer.is_moused_over()) {
        return impl.viewer.on_event(e);
    }
    return false;
}

static void draw_output_plots(osc::Simulator_screen::Impl& impl, Ui_simulation& focused, OpenSim::Component const& selected) {
    int imgui_id = 0;

    ImGui::Columns(2);
    for (auto const& ptr : selected.getOutputs()) {
        OpenSim::AbstractOutput const& ao = *ptr.second;

        ImGui::TextUnformatted(ao.getName().c_str());
        ImGui::NextColumn();

        OpenSim::Output<double> const* od_ptr =
            dynamic_cast<OpenSim::Output<double> const*>(&ao);

        if (!od_ptr) {
            ImGui::TextUnformatted(ao.getValueAsString(focused.spot_report->state).c_str());
            ImGui::NextColumn();
            continue;  // unplottable
        }

        OpenSim::Output<double> const& od = *od_ptr;

        size_t npoints = focused.regular_reports.size();
        impl.plotscratch.resize(npoints);
        size_t i = 0;
        for (auto const& report : focused.regular_reports) {
            double v = od.getValue(report->state);
            impl.plotscratch[i++] = static_cast<float>(v);
        }

        ImGui::PushID(imgui_id++);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
        ImGui::PlotLines("##nolabel", impl.plotscratch.data(), static_cast<int>(impl.plotscratch.size()));

        if (ImGui::BeginPopupContextItem(od.getName().c_str())) {
            if (ImGui::MenuItem("Add to outputs watch")) {
                impl.st->desired_outputs.emplace_back(selected.getAbsolutePath().toString(), od.getName());
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
        ImGui::NextColumn();
    }
    ImGui::Columns();
}

static void draw_selection_tab(osc::Simulator_screen::Impl& impl, Ui_simulation& focused) {
    if (!focused.selected) {
        ImGui::TextUnformatted("nothing selected");
        return;
    }
    OpenSim::Component const& selected = *focused.selected;

    ui::component_details::draw(focused.spot_report->state, focused.selected);

    if (ImGui::CollapsingHeader("outputs")) {
        draw_output_plots(impl, focused, selected);
    }
}

static void draw_outputs_tab(osc::Simulator_screen::Impl& impl, Ui_simulation& focused) {
    int imgui_id = 0;
    Main_editor_state& st = *impl.st;

    if (st.desired_outputs.empty()) {
        ImGui::TextUnformatted("No outputs being plotted: right-click them in the model editor");
        return;
    }

    ImGui::Columns(2);
    for (Desired_output const& de : st.desired_outputs) {
        ImGui::Text("%s/%s", de.component_path.c_str(), de.output_name.c_str());
        ImGui::NextColumn();

        OpenSim::Component const* cp = nullptr;
        try {
            cp = &focused.model->getComponent(de.component_path);
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
            aop = &c.getOutput(de.output_name);
        } catch (...) {
            // OpenSim, innit
        }

        if (!aop) {
            ImGui::TextUnformatted("output not found");
            ImGui::NextColumn();
            continue;
        }

        OpenSim::AbstractOutput const& ao = *aop;

        auto const* odp = dynamic_cast<OpenSim::Output<double> const*>(&ao);

        if (!odp) {
            // unplottable arbitary output
            ImGui::TextUnformatted(ao.getValueAsString(focused.spot_report->state).c_str());
            ImGui::NextColumn();
            continue;
        }

        OpenSim::Output<double> const& od = *odp;

        // else: it's a plottable output

        size_t npoints = focused.regular_reports.size();
        impl.plotscratch.resize(npoints);
        size_t i = 0;
        for (auto const& report : focused.regular_reports) {
            double v = od.getValue(report->state);
            impl.plotscratch[i++] = static_cast<float>(v);
        }

        ImGui::PushID(imgui_id++);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
        ImGui::PlotLines("##nolabel", impl.plotscratch.data(), static_cast<int>(impl.plotscratch.size()));
        ImGui::PopID();
        ImGui::NextColumn();
    }
    ImGui::Columns();
}

static void draw(osc::Simulator_screen::Impl& impl) {
    // draw main menu
    if (ImGui::BeginMainMenuBar()) {
        ui::main_menu::file_tab::draw(impl.mm_filetab_st, impl.st);
        ui::main_menu::about_tab::draw();

        if (ImGui::Button("Switch to editor (Ctrl+E)")) {

            // request the transition then exit this drawcall ASAP
            Application::current().request_screen_transition<Model_editor_screen>(std::move(impl.st));
            ImGui::EndMainMenuBar();
            return;
        }

        ImGui::EndMainMenuBar();
    }

    // edge-case: there are no simulations available, so 
    // show a "you need to run something, fool" dialog
    if (impl.st->simulations.empty()) {
        if (ImGui::Begin("Warning")) {
            ImGui::Text("No simulations are currently running");
            if (ImGui::Button("Run new simulation")) {
                action_start_simulation(*impl.st);
            }
        }
        ImGui::End();
        return;
    }

    OSC_ASSERT(!impl.st->simulations.empty() && "the simulation screen shouldn't render if there are no simulations running");

    // coerce the current simulation selection if it still has
    // its default value
    if (impl.st->focused_simulation == -1) {
        impl.st->focused_simulation = 0;
    }

    OSC_ASSERT(!impl.st->simulations.empty() && impl.st->focused_simulation != -1);

    // BEWARE: this UI element in particular enables the user to 
    // delete simulations, which can cause all kinds of havoc if
    // the renderer is midway through rendering the simulation
    //
    // so double-check the top-level assumptions and exit early if the
    // user has deleted something
    {
        if (ImGui::Begin("Simulations")) {
            draw_simulation_tab(*impl.st);
        }
        ImGui::End();

        if (impl.st->simulations.empty()) {
            impl.st->focused_simulation = -1;
            return;
        }

        if (impl.st->focused_simulation == -1) {
            impl.st->focused_simulation = 0;
        }
    }

    OSC_ASSERT(!impl.st->simulations.empty() && impl.st->focused_simulation != -1);

    // now: nothing should be able to delete/move the simulations for the
    // rest of the drawcall, so we can render whatever we like

    Ui_simulation& focused_sim =
        *impl.st->simulations.at(static_cast<size_t>(impl.st->focused_simulation));

    // draw 3d viewer
    {
        auto resp = impl.viewer.draw(
            "render",
            *focused_sim.model,
            focused_sim.spot_report->state,
            focused_sim.selected,
            focused_sim.hovered);
        if (resp.type == Response::SelectionChanged) {
            focused_sim.selected = const_cast<OpenSim::Component*>(resp.ptr);
        }
        if (resp.type == Response::HoverChanged) {
            focused_sim.hovered = const_cast<OpenSim::Component*>(resp.ptr);
        }
    }

    // draw hierarchy tab
    {
        if (ImGui::Begin("Hierarchy")) {
            auto resp = ui::component_hierarchy::draw(
                focused_sim.model.get(),
                focused_sim.selected,
                focused_sim.hovered);

            if (resp.type == ui::component_hierarchy::SelectionChanged) {
                focused_sim.selected = const_cast<OpenSim::Component*>(resp.ptr);
            } else if (resp.type == ui::component_hierarchy::HoverChanged) {
                focused_sim.hovered = const_cast<OpenSim::Component*>(resp.ptr);
            }
        }
        ImGui::End();
    }

    // draw selection tab
    {
        if (ImGui::Begin("Selection")) {
            draw_selection_tab(impl, focused_sim);
        }
        ImGui::End();
    }

    // draw outputs tab
    {
        if (ImGui::Begin("Outputs")) {
            draw_outputs_tab(impl, focused_sim);
        }
        ImGui::End();
    }

    // draw simulation stats tab
    {
        if (ImGui::Begin("Simulation stats")) {
            draw_simulation_stats_tab(impl, focused_sim);
        }
        ImGui::End();
    }

    // draw log tab
    {
        ui::log_viewer::draw(impl.log_viewer_st, "Log");
    }
}

osc::Simulator_screen::Simulator_screen(std::shared_ptr<Main_editor_state> st) :
    impl{new Impl{std::move(st)}} {
}

osc::Simulator_screen::~Simulator_screen() noexcept = default;

bool osc::Simulator_screen::on_event(SDL_Event const& e) {
    return ::on_event(*impl, e);
}

void osc::Simulator_screen::tick() {
    pop_all_simulator_updates(*impl->st);
}

void osc::Simulator_screen::draw() {
    ::draw(*impl);
}
