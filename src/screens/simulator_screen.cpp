#include "simulator_screen.hpp"

#include "src/main_editor_state.hpp"
#include "src/opensim_bindings/fd_simulation.hpp"
#include "src/ui/log_viewer.hpp"
#include "src/ui/model_viewer.hpp"
#include "src/ui/main_menu.hpp"
#include "src/screens/model_editor_screen.hpp"
#include "src/assertions.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>

#include <limits>

using namespace osc;

struct osc::Simulator_screen::Impl final {
    std::unique_ptr<Main_editor_state> st;

    osc::ui::log_viewer::State log_viewer_st;
    osc::ui::main_menu::file_tab::State mm_filetab_st;
    
    Model_viewer_widget viewer{ModelViewerWidgetFlags_Default | ModelViewerWidgetFlags_DrawFrames};

    Impl(std::unique_ptr<Main_editor_state> _st) : st {std::move(_st)} {
    }
};

static void action_start_simulation(osc::Main_editor_state& impl) {
    impl.simulations.emplace_back(new Ui_simulation{impl.model(), impl.state()});
}

static void pop_all_simulator_updates(osc::Main_editor_state& impl) {
    for (auto& simulation : impl.simulations) {
        // pop regular reports
        simulation->simulation.pop_regular_reports(simulation->regular_reports);

        // pop latest spot report
        std::unique_ptr<fd::Report> new_spot_report = simulation->simulation.try_pop_latest_report();
        if (new_spot_report) {
            simulation->spot_report = std::move(new_spot_report);
            simulation->model->realizeReport(simulation->spot_report->state);
        }
    }
}

static void draw_simulation_tab(osc::Main_editor_state& impl) {
    if (ImGui::Button("Run")) {
        action_start_simulation(impl);
    }

    for (size_t i = 0; i < impl.simulations.size(); ++i) {
        Ui_simulation& simulation = *impl.simulations[i];

        ImGui::PushID(static_cast<int>(i));
        double cur = simulation.simulation.sim_current_time().count();
        double tot = simulation.simulation.sim_final_time().count();
        float pct = static_cast<float>(cur) / static_cast<float>(tot);

        ImGui::Text("%i", i);

        ImGui::SameLine();
        if (ImGui::Button("x")) {
            impl.simulations.erase(impl.simulations.begin() + i);
            if (i <= impl.focused_simulation) {
                --impl.focused_simulation;
            }
        }

        ImGui::SameLine();
        ImGui::ProgressBar(pct);

        if (ImGui::IsItemClicked()) {
            OSC_ASSERT(i <= std::numeric_limits<int>::max());
            impl.focused_simulation = static_cast<int>(i);
        }

        ImGui::PopID();
    }
}

static bool on_event(osc::Simulator_screen::Impl& impl, SDL_Event const& e) {
    if (impl.viewer.is_moused_over()) {
        return impl.viewer.on_event(e);
    }
    return false;
}

static void draw(osc::Simulator_screen::Impl& impl) {
    // draw main menu
    if (ImGui::BeginMainMenuBar()) {
        ui::main_menu::file_tab::draw(impl.mm_filetab_st, &impl.st->model());
        ui::main_menu::about_tab::draw();

        if (ImGui::Button("Switch to editor")) {

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

    Ui_simulation const& focused_sim = 
        *impl.st->simulations.at(static_cast<size_t>(impl.st->focused_simulation));

    impl.viewer.draw("render", *focused_sim.model, focused_sim.spot_report->state);
    ui::log_viewer::draw(impl.log_viewer_st, "Log");
}

osc::Simulator_screen::Simulator_screen(std::unique_ptr<Main_editor_state> st) : 
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