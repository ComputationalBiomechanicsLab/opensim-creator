#include "show_model_screen.hpp"

#include "src/3d/gl.hpp"
#include "src/3d/3d.hpp"
#include "src/application.hpp"
#include "src/assertions.hpp"
#include "src/log.hpp"
#include "src/opensim_bindings/fd_simulation.hpp"
#include "src/screens/loading_screen.hpp"
#include "src/screens/model_editor_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/ui/component_details.hpp"
#include "src/ui/component_hierarchy.hpp"
#include "src/ui/coordinate_editor.hpp"
#include "src/ui/evenly_spaced_sparkline.hpp"
#include "src/ui/log_viewer.hpp"
#include "src/ui/main_menu.hpp"
#include "src/ui/model_viewer.hpp"
#include "src/ui/muscles_table.hpp"
#include "src/utils/helpers.hpp"
#include "src/utils/file_change_poller.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/CoordinateSet.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/Wrap/WrapObject.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SimTKcommon/basics.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace osc;
using namespace osc::ui;
using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;

static void compute_moment_arms(
    OpenSim::Muscle const& muscle,
    OpenSim::Coordinate const& c,
    SimTK::State const& base_state,
    float* out,
    size_t n) {

    SimTK::State state = base_state;
    muscle.getModel().realizeReport(state);

    bool prev_locked = c.getLocked(state);
    double prev_val = c.getValue(state);

    c.setLocked(state, false);

    double start = c.getRangeMin();
    double end = c.getRangeMax();
    double step = (end - start) / n;

    for (size_t i = 0; i < n; ++i) {
        double v = start + (i * step);
        c.setValue(state, v);
        double ma = muscle.getGeometryPath().computeMomentArm(state, c);
        out[i] = static_cast<float>(ma);
    }

    c.setLocked(state, prev_locked);
    c.setValue(state, prev_val);
}

static bool sort_components_by_name(
    OpenSim::Component const* c1,
    OpenSim::Component const* c2) {

    return c1->getName() < c2->getName();
}

struct Add_moment_arm_plot_modal_state final {
    std::vector<OpenSim::Muscle const*> muscles_scratch;
    std::vector<OpenSim::Coordinate const*> coords_scratch;
    OpenSim::Muscle const* selected_muscle = nullptr;
    OpenSim::Coordinate const* selected_coord = nullptr;
};

static void draw_add_moment_arm_plot_modal(
    Add_moment_arm_plot_modal_state& st,
    char const* modal_name,
    OpenSim::Model const& model,
    std::function<void(std::pair<OpenSim::Muscle const*, OpenSim::Coordinate const*>)> const& on_add_plot_requested) {

    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (!ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // modal not showing
        return;
    }

    ImGui::Columns(2);

    // lhs: muscle selection
    {
        ImGui::Text("muscles:");
        ImGui::Dummy({0.0f, 5.0f});

        auto& muscles = st.muscles_scratch;

        muscles.clear();
        for (OpenSim::Muscle const& musc : model.getComponentList<OpenSim::Muscle>()) {
            muscles.push_back(&musc);
        }

        // usability: sort by name
        std::sort(muscles.begin(), muscles.end(), sort_components_by_name);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild(
            "MomentArmPlotMuscleSelection", ImVec2(ImGui::GetContentRegionAvail().x, 260), false, window_flags);

        for (OpenSim::Muscle const* m : muscles) {
            if (ImGui::Selectable(m->getName().c_str(), m == st.selected_muscle)) {
                st.selected_muscle = m;
            }
        }
        ImGui::EndChild();
    }
    ImGui::NextColumn();

    // rhs: coord selection
    {
        ImGui::Text("coordinates:");
        ImGui::Dummy({0.0f, 5.0f});

        auto& coords = st.coords_scratch;

        coords.clear();
        get_coordinates(model, coords);

        // usability: sort by name
        std::sort(coords.begin(), coords.end(), sort_components_by_name);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild(
            "MomentArmPlotCoordSelection", ImVec2(ImGui::GetContentRegionAvail().x, 260), false, window_flags);

        for (OpenSim::Coordinate const* c : coords) {
            if (ImGui::Selectable(c->getName().c_str(), c == st.selected_coord)) {
                st.selected_coord = c;
            }
        }

        ImGui::EndChild();
    }
    ImGui::NextColumn();

    ImGui::Columns(1);

    if (ImGui::Button("cancel")) {
        st = {};  // reset user inputs
        ImGui::CloseCurrentPopup();
    }

    if (st.selected_coord && st.selected_muscle) {
        ImGui::SameLine();
        if (ImGui::Button("OK")) {
            on_add_plot_requested({st.selected_muscle, st.selected_coord});
            st = {};  // reset user input
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::EndPopup();
}

struct Output_plot final {
    OpenSim::AbstractOutput const* ao;
    osc::Evenly_spaced_sparkline<256> plot;

    explicit Output_plot(OpenSim::AbstractOutput const* _ao) : ao{_ao} {
    }

    void clear() {
        plot.clear();
    }

    void push_datapoint(float x, float y) {
        plot.push_datapoint(x, y);
    }

    OpenSim::AbstractOutput const* handle() const noexcept {
        return ao;
    }

    std::string const& getName() const {
        return ao->getName();
    }

    std::string const& getOwnerName() const {
        return ao->getOwner().getName();
    }
};

struct Moment_arm_plot final {
    std::string muscle_name;
    std::string coord_name;
    float x_begin;
    float x_end;
    std::array<float, 50> y_vals;
    float min;
    float max;
};

struct Coordinates_tab_data final {
    char filter[64]{};
    bool sort_by_name = true;
    bool show_rotational = true;
    bool show_translational = true;
    bool show_coupled = true;
};

struct Integrator_stat_sparkline final {
    using extrator_fn = float(fd::Stats const&);

    Evenly_spaced_sparkline<256> plot{};
    const char* name;
    extrator_fn* extractor;

    constexpr Integrator_stat_sparkline(const char* _name, extrator_fn* _extractor) :
        name{_name},
        extractor{_extractor} {
    }

    void clear() {
        plot.clear();
    }

    void push_datapoint(float x, fd::Stats const& stats) {
        plot.push_datapoint(x, extractor(stats));
    }

    void draw(float height = 50.0f) {
        plot.draw(height);
    }
};

class Selected_component final {
    OpenSim::Component const* ptr = nullptr;

public:
    // TODO: make private
    std::vector<Evenly_spaced_sparkline<512>> output_sinks;

    Selected_component& operator=(OpenSim::Component const* _ptr) {
        if (_ptr == ptr) {
            return *this;
        }

        ptr = _ptr;
        output_sinks.clear();

        if (ptr == nullptr) {
            return *this;
        }

        // if the user selects something, preallocate some output
        // sparklines for the selection

        size_t n_outputs = 0;
        for (auto const& p : ptr->getOutputs()) {
            if (dynamic_cast<OpenSim::Output<double> const*>(p.second.get())) {
                ++n_outputs;
            }
        }

        output_sinks.resize(n_outputs);

        return *this;
    }

    operator bool() const noexcept {
        return ptr != nullptr;
    }

    operator OpenSim::Component const*() const noexcept {
        return ptr;
    }

    OpenSim::Component const* operator->() const noexcept {
        return ptr;
    }

    OpenSim::Component const& operator*() const noexcept {
        return *ptr;
    }

    OpenSim::Component const* get() const noexcept {
        return ptr;
    }

    void on_ui_state_update(SimTK::State const& st) {
        // if the user currently has something selected, live-update
        // all outputs

        if (ptr == nullptr) {
            return;
        }

        float sim_time = static_cast<float>(st.getTime());

        size_t i = 0;
        for (auto const& p : ptr->getOutputs()) {
            OpenSim::AbstractOutput const* ao = p.second.get();

            // only certain types of output are plottable at the moment
            auto* o = dynamic_cast<OpenSim::Output<double> const*>(ao);
            if (o) {
                double v = o->getValue(st);
                float fv = static_cast<float>(v);
                output_sinks[i++].push_datapoint(sim_time, fv);
            }
        }
    }

    void on_user_edited_state() {
        for (auto& pl : output_sinks) {
            pl.clear();
        }
    }
};

struct Simulator_tab final {
    std::optional<fd::Simulation> simulator;

    Evenly_spaced_sparkline<256> prescribeQcalls;
    Evenly_spaced_sparkline<256> simTimeDividedByWallTime;

    std::array<Integrator_stat_sparkline, 15> integrator_plots{
        {{"accuracyInUse", [](fd::Stats const& s) { return static_cast<float>(s.accuracyInUse); }},
         {"predictedNextStepSize", [](fd::Stats const& s) { return static_cast<float>(s.predictedNextStepSize); }},
         {"numStepsAttempted", [](fd::Stats const& s) { return static_cast<float>(s.numStepsAttempted); }},
         {"numStepsTaken", [](fd::Stats const& s) { return static_cast<float>(s.numStepsTaken); }},
         {"numRealizations", [](fd::Stats const& s) { return static_cast<float>(s.numRealizations); }},
         {"numQProjections", [](fd::Stats const& s) { return static_cast<float>(s.numQProjections); }},
         {"numUProjections", [](fd::Stats const& s) { return static_cast<float>(s.numUProjections); }},
         {"numErrorTestFailures", [](fd::Stats const& s) { return static_cast<float>(s.numErrorTestFailures); }},
         {"numConvergenceTestFailures", [](fd::Stats const& s) { return static_cast<float>(s.numConvergenceTestFailures); }},
         {"numRealizationFailures", [](fd::Stats const& s) { return static_cast<float>(s.numRealizationFailures); }},
         {"numQProjectionFailures", [](fd::Stats const& s) { return static_cast<float>(s.numQProjectionFailures); }},
         {"numProjectionFailures", [](fd::Stats const& s) { return static_cast<float>(s.numProjectionFailures); }},
         {"numConvergentIterations", [](fd::Stats const& s) { return static_cast<float>(s.numConvergentIterations); }},
         {"numDivergentIterations", [](fd::Stats const& s) { return static_cast<float>(s.numDivergentIterations); }},
         {"numIterations", [](fd::Stats const& s) { return static_cast<float>(s.numIterations); }}}};

    float fd_final_time = 0.4f;
    fd::IntegratorMethod integrator_method = fd::IntegratorMethod_OpenSimManagerDefault;

    void clear() {
        prescribeQcalls.clear();
        simTimeDividedByWallTime.clear();

        for (Integrator_stat_sparkline& integrator_plot : integrator_plots) {
            integrator_plot.clear();
        }
    }

    void on_user_edited_model() {
        // if the user edits the model, kill the current simulation, because
        // it won't match what the user sees
        simulator = std::nullopt;

        clear();
    }

    void on_user_edited_state() {
        clear();
    }

    void on_ui_state_update(OpenSim::Model const&, SimTK::State const& st, fd::Stats const& stats) {
        if (!simulator) {
            return;
        }

        // get latest integrator stats
        float sim_time = static_cast<float>(st.getTime());
        float wall_time = static_cast<float>(simulator->wall_duration().count());

        prescribeQcalls.push_datapoint(sim_time, static_cast<float>(stats.numPrescribeQcalls));
        simTimeDividedByWallTime.push_datapoint(sim_time, sim_time / wall_time);

        // push 0d integrator stats onto sparklines
        for (Integrator_stat_sparkline& integrator_plot : integrator_plots) {
            integrator_plot.push_datapoint(sim_time, stats);
        }
    }

    void draw(Selected_component&, OpenSim::Model& shown_model, SimTK::State& shown_state) {
        // start/stop button
        if (simulator && simulator->is_running()) {
            ImGui::PushStyleColor(ImGuiCol_Button, {1.0f, 0.0f, 0.0f, 1.0f});
            if (ImGui::Button("stop [SPC]")) {
                simulator->request_stop();
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.6f, 0.0f, 1.0f});
            if (ImGui::Button("start [SPC]")) {
                fd::Params params{
                    shown_model,
                    shown_state,
                    std::chrono::duration<double>{static_cast<double>(fd_final_time)},
                    integrator_method
                };

                simulator.emplace(std::move(params));
            }
            ImGui::PopStyleColor();
        }

        ImGui::Dummy({0.0f, 20.0f});
        ImGui::Text("simulation config:");
        ImGui::Dummy({0.0f, 2.5f});
        ImGui::Separator();

        ImGui::Columns(2);

        ImGui::Text("final time");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("##final time float", &fd_final_time, 0.01f, 20.0f);
        ImGui::NextColumn();

        ImGui::Text("integration method");
        ImGui::NextColumn();
        {
            int method = integrator_method;
            if (ImGui::Combo(
                    "##integration method combo",
                    &method,
                    fd::integrator_method_names,
                    fd::IntegratorMethod_NumIntegratorMethods)) {
                integrator_method = static_cast<fd::IntegratorMethod>(method);
            }
        }
        ImGui::Columns();

        if (simulator) {
            std::chrono::milliseconds wall_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(simulator->wall_duration());
            double wall_secs = static_cast<double>(wall_ms.count()) / 1000.0;
            std::chrono::duration<double> sim_secs = simulator->sim_current_time();
            double frac_completed = sim_secs / simulator->sim_final_time();

            ImGui::Dummy(ImVec2{0.0f, 20.0f});
            ImGui::TextUnformatted("simulator stats:");
            ImGui::Dummy(ImVec2{0.0f, 2.5f});
            ImGui::Separator();

            ImGui::Columns(2);
            ImGui::TextUnformatted("status");
            ImGui::NextColumn();
            ImGui::TextUnformatted(simulator->status_description());
            ImGui::NextColumn();

            ImGui::TextUnformatted("progress");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::ProgressBar(static_cast<float>(frac_completed), ImVec2(0.0f, 0.0f));
            ImGui::NextColumn();

            ImGui::TextUnformatted("simulation time");
            ImGui::NextColumn();
            ImGui::Text("%.2f s", sim_secs.count());
            ImGui::NextColumn();

            ImGui::TextUnformatted("wall time");
            ImGui::NextColumn();
            ImGui::Text("%.2f s", wall_secs);
            ImGui::NextColumn();

            ImGui::TextUnformatted("sim time / wall time (avg.)");
            ImGui::NextColumn();
            ImGui::Text("%.3f", (sim_secs / wall_secs).count());
            ImGui::NextColumn();

            ImGui::TextUnformatted("Reports popped");
            ImGui::NextColumn();
            ImGui::Text("%i", simulator->num_latest_reports_popped());
            ImGui::NextColumn();

            ImGui::TextUnformatted("States collected");
            ImGui::NextColumn();
            static std::vector<std::unique_ptr<osc::fd::Report>> reports;
            simulator->pop_regular_reports(reports);
            ImGui::Text("%zu", reports.size());
            ImGui::NextColumn();

            ImGui::Columns();

            ImGui::Dummy({0.0f, 20.0f});
            ImGui::TextUnformatted("plots:");
            ImGui::Dummy({0.0f, 2.5f});
            ImGui::Separator();

            ImGui::Columns(2);

            ImGui::TextUnformatted("prescribeQcalls");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            prescribeQcalls.draw(30.0f);
            ImGui::NextColumn();

            ImGui::TextUnformatted("sim time / wall time");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            simTimeDividedByWallTime.draw(30.0f);
            ImGui::NextColumn();

            for (Integrator_stat_sparkline& integrator_plot : integrator_plots) {
                ImGui::TextUnformatted(integrator_plot.name);
                ImGui::NextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                integrator_plot.draw(30.0f);
                ImGui::NextColumn();
            }

            ImGui::Columns();
        }
    }
};

struct Momentarms_tab_data final {
    std::string const* selected_musc = nullptr;
    std::string const* selected_coord = nullptr;
    std::vector<std::unique_ptr<Moment_arm_plot>> plots;
};

struct Outputs_tab_data final {
    char filter[64]{};
    std::vector<OpenSim::AbstractOutput const*> available;
    std::optional<OpenSim::AbstractOutput const*> selected;
    std::vector<OpenSim::AbstractOutput const*> watches;
    std::vector<Output_plot> plots;

    void on_ui_state_update(SimTK::State const& st) {
        float sim_millis = 1000.0f * static_cast<float>(st.getTime());

        for (Output_plot& p : plots) {

            // only certain types of output are plottable at the moment
            auto* o = dynamic_cast<OpenSim::Output<double> const*>(p.handle());
            OSC_ASSERT(o != nullptr && "unexpected output type (expected OpenSim::Output<double>)");
            double v = o->getValue(st);
            float fv = static_cast<float>(v);

            p.push_datapoint(sim_millis, fv);
        }
    }

    void on_user_edited_model() {
        selected = std::nullopt;
        plots.clear();
    }

    void on_user_edited_state() {
        for (Output_plot& p : plots) {
            p.clear();
        }
    }
};

struct Show_model_screen::Impl final {
    std::unique_ptr<OpenSim::Model> model;
    SimTK::State latest_state;

    Selected_component selected_component;
    std::array<Model_viewer_widget, 2> model_viewers = {
        Model_viewer_widget{Application::current().get_gpu_storage(), ModelViewerWidgetFlags_Default | ModelViewerWidgetFlags_CanOnlyInteractWithMuscles},
        Model_viewer_widget{Application::current().get_gpu_storage(), ModelViewerWidgetFlags_Default | ModelViewerWidgetFlags_CanOnlyInteractWithMuscles},
    };
    OpenSim::Component const* current_hover = nullptr;

    ui::main_menu::file_tab::State mm_filetab_st;

    coordinate_editor::State coords_tab_st;
    muscles_table::State muscles_table_st;
    log_viewer::State log_viewer_st;

    Simulator_tab simulator_tab;
    Momentarms_tab_data mas_tab;
    Outputs_tab_data outputs_tab;
    Add_moment_arm_plot_modal_state add_moment_arm_modal_st;

    File_change_poller file_poller;

    Impl(std::unique_ptr<OpenSim::Model> _model) :
        model{std::move(_model)},
        latest_state{[this]() {
            model->finalizeFromProperties();
            SimTK::State s = model->initSystem();
            model->realizeReport(s);
            return s;
        }()},
        file_poller{1000ms, model->getDocumentFileName()} {
    }

    bool action_try_reload_model_file() {
        std::string file = model->getDocumentFileName();
        if (!file.empty()) {
            model = std::make_unique<OpenSim::Model>(file);
            on_user_edited_model();
            return true;
        }
        return false;
    }

    void action_reset_model_to_initial_state() {
        // R: reset the model to its initial state
        latest_state = model->initSystem();
        on_user_edited_state();
    }

    void action_toggle_sim_start_or_stop() {
        bool sim_running =
            simulator_tab.simulator && simulator_tab.simulator->is_running();

        if (sim_running) {
            simulator_tab.simulator->request_stop();
        } else {
            fd::Params p{
                *model,
                latest_state,
                std::chrono::duration<double>{static_cast<double>(simulator_tab.fd_final_time)},
                simulator_tab.integrator_method
            };

            simulator_tab.simulator.emplace(std::move(p));
        }
    }

    void action_switch_to_editor() {
        Application::current().request_screen_transition<Model_editor_screen>(std::make_unique<OpenSim::Model>(*model));
    }

    void action_transition_to_splash_screen() {
        Application::current().request_screen_transition<Splash_screen>();
    }

    void action_clear_selection() {
        selected_component = nullptr;
    }

    void action_quit_application() {
        Application::current().request_quit_application();
    }

    void action_double_simulation_time() {
        simulator_tab.fd_final_time *= 2.0f;
    }

    void action_half_simulation_time() {
        simulator_tab.fd_final_time /= 2.0f;
    }

    bool handle_keyboard_event(SDL_KeyboardEvent const& e) {
        if (e.keysym.mod & KMOD_CTRL) {
            switch (e.keysym.sym) {
            case SDLK_r:
                return action_try_reload_model_file();
            case SDLK_e:
                action_switch_to_editor();
                return true;
            case SDLK_q:
                action_quit_application();
                return true;
            case SDLK_a:
                action_clear_selection();
                return true;
            }
        }

        // unmodified keybinds
        switch (e.keysym.sym) {
        case SDLK_r:
            action_reset_model_to_initial_state();
            return true;
        case SDLK_SPACE:
            action_toggle_sim_start_or_stop();
            return true;
        case SDLK_EQUALS:
            action_double_simulation_time();
            return true;
        case SDLK_MINUS:
            action_half_simulation_time();
            return true;
        }

        return false;
    }

    void handle_mouseup_event(SDL_MouseButtonEvent const& e) {
        // maybe they're trying to select something in the viewport, so check if they are
        // hovered over a component and select it if they are
        if (e.button == SDL_BUTTON_RIGHT && current_hover) {
            selected_component = current_hover;
        }
    }

    // handle top-level UI event (user click, user drag, etc.)
    bool handle_event(SDL_Event const& e) {
        bool handled = false;

        switch (e.type) {
        case SDL_KEYDOWN:
            handled = handle_keyboard_event(e.key);
            break;
        case SDL_MOUSEBUTTONUP:
            handle_mouseup_event(e.button);  // doesn't affect handling
            break;
        }

        if (handled) {
            return handled;
        }

        // if we've got this far, the event is still unhandled: try and propagate it to
        // each model viewer - if the model viewer is moused over
        for (auto& viewer : model_viewers) {
            if (viewer.is_moused_over()) {
                viewer.on_event(e);
                return true;
            }
        }

        return false;
    }

    // "tick" the UI state (usually, used for updating animations etc.)
    void tick() {
        // grab the latest state (if any) from the simulator and (if updated)
        // update the UI to reflect the latest state
        if (simulator_tab.simulator) {
            std::unique_ptr<fd::Report> latest_report = simulator_tab.simulator->try_pop_latest_report();

            if (latest_report) {
                latest_state = std::move(latest_report->state);

                model->realizeReport(latest_state);
                outputs_tab.on_ui_state_update(latest_state);
                simulator_tab.on_ui_state_update(*model, latest_state, latest_report->stats);
                selected_component.on_ui_state_update(latest_state);
            }
        }

        if (file_poller.change_detected(model->getDocumentFileName())) {
            std::unique_ptr<OpenSim::Model> p;
            try {
                p = std::make_unique<OpenSim::Model>(model->getDocumentFileName());
            } catch (std::exception const& ex) {
                log::error("an error occurred while trying to automatically load a model file");
                log::error(ex.what());
            }

            if (p) {
                model = std::move(p);
                on_user_edited_model();
            }
        }
    }

    void on_user_edited_model() {
        // these might be invalidated by changing the model because they might
        // contain (e.g.) pointers into the model
        mas_tab.selected_musc = nullptr;
        mas_tab.selected_coord = nullptr;
        selected_component = nullptr;

        outputs_tab.on_user_edited_model();
        simulator_tab.on_user_edited_model();

        latest_state = model->initSystem();
        model->realizeReport(latest_state);
    }

    void on_user_edited_state() {
        // kill the simulator whenever a user-initiated state change happens
        simulator_tab.simulator = std::nullopt;

        model->realizeReport(latest_state);

        outputs_tab.on_user_edited_state();
        simulator_tab.on_user_edited_state();
        selected_component.on_user_edited_state();
    }

    bool simulator_running() {
        return simulator_tab.simulator && simulator_tab.simulator->is_running();
    }

    void draw_main_menu_actions_tab() {
        if (ImGui::BeginMenu("Actions")) {
            if (ImGui::MenuItem("Start/Stop Simulation", "Space")) {
                action_toggle_sim_start_or_stop();
            }

            if (ImGui::MenuItem("Reset Model to Initial State", "R")) {
                action_reset_model_to_initial_state();
            }

            if (ImGui::MenuItem("Reload Model File", "Ctrl+R")) {
                action_try_reload_model_file();
            }

            if (ImGui::MenuItem("Clear Selection", "Ctrl+A")) {
                action_clear_selection();
            }

            if (ImGui::MenuItem("Switch to Editor", "Ctrl+E")) {
                action_switch_to_editor();
            }

            if (ImGui::MenuItem("Half Simulation Time", "-")) {
                action_half_simulation_time();
            }

            if (ImGui::MenuItem("Double Simulation Time", "=")) {
                action_double_simulation_time();
            }

            ImGui::EndMenu();
        }
    }

    // draw a frame of the UI
    void draw() {
        // draw top menu bar
        if (ImGui::BeginMainMenuBar()) {
            ui::main_menu::file_tab::draw(mm_filetab_st);
            this->draw_main_menu_actions_tab();
            ui::main_menu::about_tab::draw();

            if (ImGui::Button("Switch to editor (Ctrl+E)")) {
                Application::current().request_screen_transition<Model_editor_screen>(
                    std::make_unique<OpenSim::Model>(*model));
            }

            ImGui::EndMainMenuBar();
        }

        // draw model viewer(s)
        for (size_t i = 0; i < model_viewers.size(); ++i) {

            char buf[64];
            std::snprintf(buf, sizeof(buf), "viewer_%zu", i);

            auto& viewer = model_viewers[i];
            auto on_selection_change =
                [&viewer, &selected_component = selected_component](OpenSim::Component const* new_selection) {
                    if (viewer.is_moused_over()) {
                        selected_component = new_selection;
                    }
                };
            auto on_hover_change = [&viewer, &current_hover = current_hover](OpenSim::Component const* new_hover) {
                if (viewer.is_moused_over()) {
                    current_hover = new_hover;
                }
            };

            viewer.draw(
                buf, *model, latest_state, selected_component, current_hover, on_selection_change, on_hover_change);
        }

        if (ImGui::Begin("Hierarchy")) {
            auto resp = component_hierarchy::draw(&model->getRoot(), selected_component.get(), current_hover);

            switch (resp.type) {
            case component_hierarchy::SelectionChanged:
                selected_component = resp.ptr;
                break;
            case component_hierarchy::HoverChanged:
                current_hover = resp.ptr;
                break;
            default:
                break;
            }
        }
        ImGui::End();

        if (ImGui::Begin("Muscles")) {
            auto resp = muscles_table::draw(muscles_table_st, *model, latest_state);
            switch (resp.type) {
            case muscles_table::Response::SelectionChanged:
                selected_component = resp.ptr;
                break;
            case muscles_table::Response::HoverChanged:
                current_hover = resp.ptr;
                break;
            default:
                break;
            }
        }
        ImGui::End();

        if (ImGui::Begin("Outputs")) {
            draw_outputs_tab();
        }
        ImGui::End();

        if (ImGui::Begin("Moment Arms")) {
            draw_moment_arms_tab();
        }
        ImGui::End();

        if (ImGui::Begin("Selection")) {
            draw_selection_tab();
        }
        ImGui::End();

        if (ImGui::Begin("Coordinates")) {
            if (ui::coordinate_editor::draw(coords_tab_st, *model, latest_state)) {
                on_user_edited_state();
            }
        }
        ImGui::End();

        if (ImGui::Begin("Simulate")) {
            simulator_tab.draw(selected_component, *model, latest_state);
        }
        ImGui::End();

        ui::log_viewer::draw(log_viewer_st, "Log");
    }

    void on_user_wants_to_add_ma_plot(std::pair<OpenSim::Muscle const*, OpenSim::Coordinate const*> pair) {
        auto plot = std::make_unique<Moment_arm_plot>();
        plot->muscle_name = pair.first->getName();
        plot->coord_name = pair.second->getName();
        plot->x_begin = static_cast<float>(pair.second->getRangeMin());
        plot->x_end = static_cast<float>(pair.second->getRangeMax());

        // populate y values
        compute_moment_arms(*pair.first, *pair.second, latest_state, plot->y_vals.data(), plot->y_vals.size());
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();
        for (float v : plot->y_vals) {
            min = std::min(min, v);
            max = std::max(max, v);
        }
        plot->min = min;
        plot->max = max;

        mas_tab.plots.push_back(std::move(plot));
    }

    void draw_moment_arms_tab() {
        // let user open a modal for adding new coordinate plots
        {
            static constexpr char const* modal_name = "add_ma_modal";

            if (ImGui::Button("add plot")) {
                ImGui::OpenPopup(modal_name);
            }

            draw_add_moment_arm_plot_modal(
                add_moment_arm_modal_st, modal_name, *model, [&](auto p) { on_user_wants_to_add_ma_plot(p); });
        }

        if (!mas_tab.plots.empty() && ImGui::Button("clear all")) {
            mas_tab.plots.clear();
        }

        ImGui::Separator();

        ImGui::Columns(2);
        for (size_t i = 0; i < mas_tab.plots.size(); ++i) {
            Moment_arm_plot const& p = *mas_tab.plots[i];
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::PlotLines(
                "",
                p.y_vals.data(),
                static_cast<int>(p.y_vals.size()),
                0,
                nullptr,
                std::numeric_limits<float>::min(),
                std::numeric_limits<float>::max(),
                ImVec2(0, 100.0f));
            ImGui::NextColumn();
            ImGui::Text("muscle: %s", p.muscle_name.c_str());
            ImGui::Text("coord : %s", p.coord_name.c_str());
            ImGui::Text("min   : %f", static_cast<double>(p.min));
            ImGui::Text("max   : %f", static_cast<double>(p.max));
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Button("delete")) {
                auto it = mas_tab.plots.begin() + static_cast<int>(i);
                mas_tab.plots.erase(it, it + 1);
            }
            ImGui::PopID();
            ImGui::NextColumn();
        }
        ImGui::Columns();
    }

    void draw_outputs_tab() {
        outputs_tab.available.clear();

        // get available outputs from model
        {
            // model-level outputs (e.g. kinetic energy)
            for (auto const& p : model->getOutputs()) {
                outputs_tab.available.push_back(p.second.get());
            }

            // muscle outputs
            for (auto const& musc : model->getComponentList<OpenSim::Muscle>()) {
                for (auto const& p : musc.getOutputs()) {
                    outputs_tab.available.push_back(p.second.get());
                }
            }
        }

        char buf[1024];

        // apply user filters
        {
            auto it = std::remove_if(
                outputs_tab.available.begin(), outputs_tab.available.end(), [&](OpenSim::AbstractOutput const* ao) {
                    std::snprintf(
                        buf,
                        sizeof(buf),
                        "%s/%s",
                        ao->getOwner().getName().c_str(),
                        ao->getName().c_str());
                    return std::strstr(buf, outputs_tab.filter) == nullptr;
                });
            outputs_tab.available.erase(it, outputs_tab.available.end());
        }

        // input: filter selectable outputs
        ImGui::InputText("filter", outputs_tab.filter, sizeof(outputs_tab.filter));
        ImGui::Text("%zu available outputs", outputs_tab.available.size());

        // list of selectable outputs
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
        if (ImGui::BeginChild("AvailableOutputsSelection", ImVec2(0.0f, 150.0f), true, window_flags)) {
            for (auto const& ao : outputs_tab.available) {
                snprintf(
                    buf,
                    sizeof(buf),
                    "%s/%s",
                    ao->getOwner().getName().c_str(),
                    ao->getName().c_str());
                if (ImGui::Selectable(buf, ao == outputs_tab.selected)) {
                    outputs_tab.selected = ao;
                }
            }
        }
        ImGui::EndChild();

        // buttons: "watch" and "plot"
        if (outputs_tab.selected) {
            OpenSim::AbstractOutput const* selected = outputs_tab.selected.value();

            // all outputs can be "watch"ed
            if (ImGui::Button("watch selected")) {
                outputs_tab.watches.push_back(selected);
                outputs_tab.selected = std::nullopt;
            }

            // only some outputs can be plotted
            if (dynamic_cast<OpenSim::Output<double> const*>(selected) != nullptr) {
                ImGui::SameLine();
                if (ImGui::Button("plot selected")) {
                    outputs_tab.plots.emplace_back(selected);
                    outputs_tab.selected = std::nullopt;
                }
            }
        }

        // draw watches
        if (!outputs_tab.watches.empty()) {
            ImGui::Text("watches:");
            ImGui::Separator();
            for (OpenSim::AbstractOutput const* ao : outputs_tab.watches) {
                std::string v = ao->getValueAsString(latest_state);
                ImGui::Text("    %s/%s: %s", ao->getOwner().getName().c_str(), ao->getName().c_str(), v.c_str());
            }
        }

        // draw plots
        if (!outputs_tab.plots.empty()) {
            ImGui::Text("plots:");
            ImGui::Separator();

            ImGui::Columns(2);
            for (Output_plot const& p : outputs_tab.plots) {
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
                p.plot.draw();
                ImGui::NextColumn();
                ImGui::Text("%s/%s", p.getOwnerName().c_str(), p.getName().c_str());
                ImGui::Text("min: %.3f", static_cast<double>(p.plot.min));
                ImGui::Text("max: %.3f", static_cast<double>(p.plot.max));
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
    }

    void draw_selection_tab() {
        if (!selected_component) {
            ImGui::Text("nothing selected: right click something");
            return;
        }

        // draw standard selection info
        if (auto resp = ui::component_details::draw(latest_state, selected_component.get());
            resp.type == component_details::SelectionChanged) {
            selected_component = resp.ptr;
        }

        // draw selection outputs (screen-specific)

        OpenSim::Component const& c = *selected_component;

        // outputs
        if (ImGui::CollapsingHeader("outputs")) {
            size_t i = 0;
            for (auto const& ptr : c.getOutputs()) {
                OpenSim::AbstractOutput const* ao = ptr.second.get();

                ImGui::Columns(2);

                ImGui::Text("%s", ao->getName().c_str());
                ImGui::PushStyleColor(ImGuiCol_Text, 0xaaaaaaaa);
                ImGui::Text("%s", ao->getValueAsString(latest_state).c_str());
                ImGui::PopStyleColor();
                ImGui::NextColumn();

                OpenSim::Output<double> const* od = dynamic_cast<OpenSim::Output<double> const*>(ao);
                if (od) {
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
                    selected_component.output_sinks[i++].draw(25.0f);

                    if (ImGui::BeginPopupContextItem(od->getName().c_str())) {
                        if (ImGui::MenuItem("Add to outputs tab")) {
                            outputs_tab.plots.emplace_back(ao);
                        }

                        ImGui::EndPopup();
                    }
                }
                ImGui::NextColumn();

                ImGui::Columns();
                ImGui::Separator();
            }
        }
    }
};

// public API

Show_model_screen::Show_model_screen(std::unique_ptr<OpenSim::Model> model) : impl{new Impl{std::move(model)}} {
}

Show_model_screen::~Show_model_screen() noexcept {
    delete impl;
}

bool Show_model_screen::on_event(SDL_Event const& e) {
    return impl->handle_event(e);
}

void Show_model_screen::tick() {
    return impl->tick();
}

void Show_model_screen::draw() {
    impl->draw();
}
