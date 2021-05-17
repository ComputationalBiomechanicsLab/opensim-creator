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

static bool sort_components_by_name(
    OpenSim::Component const* c1,
    OpenSim::Component const* c2) {

    return c1->getName() < c2->getName();
}

static void ma_compute_momentarms(
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

struct Ma_add_plot_modal_state final {
    std::vector<OpenSim::Muscle const*> muscles_scratch;
    std::vector<OpenSim::Coordinate const*> coords_scratch;
    OpenSim::Muscle const* selected_muscle = nullptr;
    OpenSim::Coordinate const* selected_coord = nullptr;
};

struct Ma_add_plot_modal_response final {
    OpenSim::Muscle const* muscle = nullptr;
    OpenSim::Coordinate const* coord = nullptr;
};

static Ma_add_plot_modal_response ma_add_plot_modal_draw(
    Ma_add_plot_modal_state& st,
    char const* modal_name,
    OpenSim::Model const& model) {

    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    Ma_add_plot_modal_response rv;

    // try to show modal
    if (!ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // modal not showing
        return rv;
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
            rv.muscle = st.selected_muscle;
            rv.coord = st.selected_coord;
            st = {};  // reset user input
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::EndPopup();

    return rv;
}

struct Ma_plot final {
    std::string muscle_name;
    std::string coord_name;
    float x_begin;
    float x_end;
    std::array<float, 50> y_vals;
    float min;
    float max;
};

struct Ma_tab_state final {
    std::vector<std::unique_ptr<Ma_plot>> plots;
};

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
            /*
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
            */
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

struct Outputs_tab_state final {
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

// translate a pointer to a component in model A to a pointer to a component in model B
//
// returns nullptr if the pointer cannot be cleanly translated
static OpenSim::Component const*
    relocate_component_pointer_to_new_model(OpenSim::Model const& model, OpenSim::Component const* ptr) {

    if (!ptr) {
        return nullptr;
    }

    try {
        return const_cast<OpenSim::Component*>(model.findComponent(ptr->getAbsolutePath()));
    } catch (OpenSim::Exception const&) {
        // finding fails with exception when ambiguous (fml)
        return nullptr;
    }
}

struct Ui_simulation final {
    // simulation-side: a simulation running on a background thread
    fd::Simulation simulation;

    // UI-side: copy of the simulation-side model
    std::unique_ptr<OpenSim::Model> model;

    // UI-side: spot report: latest (usually per-integration-step) report
    // popped from the simulator
    std::unique_ptr<fd::Report> spot_report;

    // UI-side: regular reports popped from the simulator
    //
    // the simulator is guaranteed to produce reports at some regular
    // interval (in simulation time). These are what should be plotted
    // etc.
    std::vector<std::unique_ptr<fd::Report>> regular_reports;
};

struct Show_model_screen::Impl final {
    // edited model + state
    std::unique_ptr<OpenSim::Model> edited_model;
    SimTK::State edited_state = [this]() {
        edited_model->finalizeFromProperties();
        SimTK::State s = edited_model->initSystem();
        edited_model->realizeReport(s);
        return s;
    }();

    // simulation models + states
    std::vector<Ui_simulation> simulations;

    // 3D viewers
    std::array<Model_viewer_widget, 2> model_viewers = {
        Model_viewer_widget{
            Application::current().get_gpu_storage(),
            ModelViewerWidgetFlags_Default | ModelViewerWidgetFlags_CanOnlyInteractWithMuscles},
        Model_viewer_widget{
            Application::current().get_gpu_storage(),
            ModelViewerWidgetFlags_Default | ModelViewerWidgetFlags_CanOnlyInteractWithMuscles},
    };

    ui::main_menu::file_tab::State mm_filetab_st;
    coordinate_editor::State coords_tab_st;
    muscles_table::State muscles_table_st;
    log_viewer::State log_viewer_st;
    // Simulator_tab simulator_tab;
    Outputs_tab_state outputs_tab;
    Ma_tab_state mas_tab;
    Ma_add_plot_modal_state add_moment_arm_modal_st;

    File_change_poller file_poller{1000ms, edited_model->getDocumentFileName()};

    int selected_idx = -1;
    OpenSim::Component const* selected_component = nullptr;
    OpenSim::Component const* hovered_component = nullptr;

    // helpers

    // set which model is selected in the GUI while maintaining other invariants
    // (e.g. update pointers)

    OpenSim::Model& active_model() {
        if (selected_idx == -1) {
            return *edited_model;
        } else {
            return *simulations.at(selected_idx).model;
        }
    }

    SimTK::State& active_state() {
        if (selected_idx == -1) {
            return edited_state;
        } else {
            return simulations.at(selected_idx).spot_report->state;
        }
    }

    void select_model(int idx) {
        selected_idx = idx;
        OpenSim::Model& m = active_model();
        selected_component = relocate_component_pointer_to_new_model(m, selected_component);
        hovered_component = relocate_component_pointer_to_new_model(m, hovered_component);
    }

    Impl(std::unique_ptr<OpenSim::Model> _model) : edited_model{std::move(_model)} {
    }
};

static void action_reset_model_to_initial_state(Show_model_screen::Impl& impl) {
    impl.edited_state = impl.edited_model->initSystem();
}

static void action_switch_to_editor(Show_model_screen::Impl& impl) {
    auto copy = std::make_unique<OpenSim::Model>(*impl.edited_model);
    Application::current().request_screen_transition<Model_editor_screen>(std::move(copy));
}

static void action_clear_selection(Show_model_screen::Impl& impl) {
    impl.selected_component = nullptr;
}

static void action_quit_application() {
    Application::current().request_quit_application();
}

static bool action_try_reload_model_file(Show_model_screen::Impl& impl) {
    std::string file = impl.edited_model->getDocumentFileName();

    if (file.empty()) {
        return false;
    }

    std::unique_ptr<OpenSim::Model> model = nullptr;

    try {
        model = std::make_unique<OpenSim::Model>(file);
    } catch (std::exception& ex) {
        log::error("error reloading model: %s", ex.what());
        return false;
    }

    if (!model) {
        return false;
    }

    impl.selected_component = relocate_component_pointer_to_new_model(*model, impl.selected_component);
    impl.hovered_component = relocate_component_pointer_to_new_model(*model, impl.hovered_component);
    impl.edited_model = std::move(model);
    impl.edited_state = impl.edited_model->initSystem();
    impl.edited_model->realizeReport(impl.edited_state);
    impl.selected_idx = -1;

    return true;
}

static void action_start_simulation(Show_model_screen::Impl& impl) {
    // make a copy of the current (edited) model for the GUI
    auto gui_model = std::make_unique<OpenSim::Model>(*impl.edited_model);
    gui_model->finalizeFromProperties();
    SimTK::State& guistate = gui_model->initSystem();

    // make a "fake" report so that the simulator at least starts with
    // one report before the first spot report is sent by the simulator
    // thread
    std::unique_ptr<fd::Report> report = std::make_unique<fd::Report>();
    report->state = guistate;
    report->stats = {};
    gui_model->realizeReport(report->state);

    // the screen's `impl.state` may contain user edits, like coordinate edits,
    // etc. so a copy of that should be used as the initial state, rather than the
    // model's default initial state
    std::unique_ptr<SimTK::State> simstate = std::make_unique<SimTK::State>(impl.edited_state);
    auto sim_model = std::make_unique<OpenSim::Model>(*impl.edited_model);
    sim_model->initSystem();
    sim_model->setPropertiesFromState(*simstate);
    sim_model->realizePosition(*simstate);
    sim_model->equilibrateMuscles(*simstate);
    sim_model->realizeAcceleration(*simstate);

    fd::Params params{std::move(sim_model), std::move(simstate)};
    params.final_time = std::chrono::duration<double>{0.4};
    fd::Simulation sim{std::move(params)};

    Ui_simulation uisim{std::move(sim), std::move(gui_model), std::move(report), {}};
    impl.simulations.push_back(std::move(uisim));

    // change focus to newly-started simulation
    impl.select_model(static_cast<int>(impl.simulations.size()) - 1);
}

static bool handle_keyboard_event(Show_model_screen::Impl& impl, SDL_KeyboardEvent const& e) {
    if (e.keysym.mod & KMOD_CTRL) {
        switch (e.keysym.sym) {
        case SDLK_r:
            return action_try_reload_model_file(impl);
        case SDLK_e:
            action_switch_to_editor(impl);
            return true;
        case SDLK_q:
            action_quit_application();
            return true;
        case SDLK_a:
            action_clear_selection(impl);
            return true;
        }
    }

    // unmodified keybinds
    switch (e.keysym.sym) {
    case SDLK_r:
        action_reset_model_to_initial_state(impl);
        return true;
    case SDLK_SPACE:
        action_start_simulation(impl);
        return true;
    /* TODO
    case SDLK_EQUALS:
        action_double_simulation_time(impl);
        return true;
    case SDLK_MINUS:
        action_half_simulation_time(impl);
        return true;
    */
    }

    return false;
}

static void handle_mouseup_event(Show_model_screen::Impl& impl, SDL_MouseButtonEvent const& e) {
    // maybe they're trying to select something in the viewport, so check if they are
    // hovered over a component and select it if they are
    if (e.button == SDL_BUTTON_RIGHT && impl.hovered_component) {
        impl.selected_component = impl.hovered_component;
    }
}

// handle top-level UI event (user click, user drag, etc.)
static bool handle_event(Show_model_screen::Impl& impl, SDL_Event const& e) {
    bool handled = false;

    switch (e.type) {
    case SDL_KEYDOWN:
        handled = handle_keyboard_event(impl, e.key);
        break;
    case SDL_MOUSEBUTTONUP:
        handle_mouseup_event(impl, e.button);  // doesn't affect handling
        break;
    }

    if (handled) {
        return handled;
    }

    // if we've got this far, the event is still unhandled: try and propagate it to
    // each model viewer - if the model viewer is moused over
    for (auto& viewer : impl.model_viewers) {
        if (viewer.is_moused_over()) {
            viewer.on_event(e);
            return true;
        }
    }

    return false;
}

static void check_for_backing_file_changes(Show_model_screen::Impl& impl) {
    std::string filename = impl.edited_model->getDocumentFileName();

    if (!impl.file_poller.change_detected(filename)) {
        return;
    }

    std::unique_ptr<OpenSim::Model> p = nullptr;

    try {
        p = std::make_unique<OpenSim::Model>(filename);
    } catch (std::exception const& ex) {
        log::error("an error occurred while trying to automatically load a model file");
        log::error(ex.what());
        return;
    }

    if (p == nullptr) {
        return;
    }

    impl.edited_state = p->initSystem();
    p->realizeReport(impl.edited_state);

    std::swap(p, impl.edited_model);
    impl.selected_idx = -1;
    impl.selected_component = relocate_component_pointer_to_new_model(*impl.edited_model, impl.selected_component);
    impl.hovered_component = relocate_component_pointer_to_new_model(*impl.edited_model, impl.hovered_component);
}

static void pop_all_simulator_updates(Show_model_screen::Impl& impl) {
    for (auto& simulation : impl.simulations) {
        // pop regular reports
        simulation.simulation.pop_regular_reports(simulation.regular_reports);

        // pop latest spot report
        std::unique_ptr<fd::Report> new_spot_report = simulation.simulation.try_pop_latest_report();
        if (new_spot_report) {
            simulation.spot_report = std::move(new_spot_report);
            simulation.model->realizeReport(simulation.spot_report->state);
        }
    }
}

// "tick" the UI state (usually, used for updating animations etc.)
static void tick(Show_model_screen::Impl& impl) {
    pop_all_simulator_updates(impl);
    check_for_backing_file_changes(impl);
}

static void on_user_wants_to_add_ma_plot(Show_model_screen::Impl& impl, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& coord) {
    auto plot = std::make_unique<Ma_plot>();
    plot->muscle_name = muscle.getName();
    plot->coord_name = coord.getName();
    plot->x_begin = static_cast<float>(coord.getRangeMin());
    plot->x_end = static_cast<float>(coord.getRangeMax());

    // populate y values
    ma_compute_momentarms(muscle, coord, impl.active_state(), plot->y_vals.data(), plot->y_vals.size());
    float min = std::numeric_limits<float>::max();
    float max = std::numeric_limits<float>::min();
    for (float v : plot->y_vals) {
        min = std::min(min, v);
        max = std::max(max, v);
    }
    plot->min = min;
    plot->max = max;

    impl.mas_tab.plots.push_back(std::move(plot));
}

static void draw_moment_arms_tab(Show_model_screen::Impl& impl) {
    // let user open a modal for adding new coordinate plots
    {
        static constexpr char const* modal_name = "add_ma_modal";

        if (ImGui::Button("add plot")) {
            ImGui::OpenPopup(modal_name);
        }

        auto resp = ma_add_plot_modal_draw(impl.add_moment_arm_modal_st, modal_name, impl.active_model());
        if (resp.muscle && resp.coord) {
            on_user_wants_to_add_ma_plot(impl, *resp.muscle, *resp.coord);
        }
    }

    if (!impl.mas_tab.plots.empty() && ImGui::Button("clear all")) {
        impl.mas_tab.plots.clear();
    }

    ImGui::Separator();

    ImGui::Columns(2);
    for (size_t i = 0; i < impl.mas_tab.plots.size(); ++i) {
        Ma_plot const& p = *impl.mas_tab.plots[i];
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
            auto it = impl.mas_tab.plots.begin() + static_cast<int>(i);
            impl.mas_tab.plots.erase(it, it + 1);
        }
        ImGui::PopID();
        ImGui::NextColumn();
    }
    ImGui::Columns();
}

/* TODO
static void draw_outputs_tab(Show_model_screen::Impl& impl) {
    impl.outputs_tab.available.clear();

    // get available outputs from model
    {
        // model-level outputs (e.g. kinetic energy)
        for (auto const& p : impl.model->getOutputs()) {
           impl. outputs_tab.available.push_back(p.second.get());
        }

        // muscle outputs
        for (auto const& musc : impl.model->getComponentList<OpenSim::Muscle>()) {
            for (auto const& p : musc.getOutputs()) {
               impl.outputs_tab.available.push_back(p.second.get());
            }
        }
    }

    char buf[1024];

    // apply user filters
    {
        auto it = std::remove_if(
            impl.outputs_tab.available.begin(), impl.outputs_tab.available.end(), [&](OpenSim::AbstractOutput const* ao) {
                std::snprintf(
                    buf,
                    sizeof(buf),
                    "%s/%s",
                    ao->getOwner().getName().c_str(),
                    ao->getName().c_str());
                return std::strstr(buf, impl.outputs_tab.filter) == nullptr;
            });
        impl.outputs_tab.available.erase(it, impl.outputs_tab.available.end());
    }

    // input: filter selectable outputs
    ImGui::InputText("filter", impl.outputs_tab.filter, sizeof(impl.outputs_tab.filter));
    ImGui::Text("%zu available outputs", impl.outputs_tab.available.size());

    // list of selectable outputs
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    if (ImGui::BeginChild("AvailableOutputsSelection", ImVec2(0.0f, 150.0f), true, window_flags)) {
        for (auto const& ao : impl.outputs_tab.available) {
            snprintf(
                buf,
                sizeof(buf),
                "%s/%s",
                ao->getOwner().getName().c_str(),
                ao->getName().c_str());
            if (ImGui::Selectable(buf, ao == impl.outputs_tab.selected)) {
                impl.outputs_tab.selected = ao;
            }
        }
    }
    ImGui::EndChild();

    // buttons: "watch" and "plot"
    if (impl.outputs_tab.selected) {
        OpenSim::AbstractOutput const* selected = impl.outputs_tab.selected.value();

        // all outputs can be "watch"ed
        if (ImGui::Button("watch selected")) {
            impl.outputs_tab.watches.push_back(selected);
            impl.outputs_tab.selected = std::nullopt;
        }

        // only some outputs can be plotted
        if (dynamic_cast<OpenSim::Output<double> const*>(selected) != nullptr) {
            ImGui::SameLine();
            if (ImGui::Button("plot selected")) {
                impl.outputs_tab.plots.emplace_back(selected);
                impl.outputs_tab.selected = std::nullopt;
            }
        }
    }

    // draw watches
    if (!impl.outputs_tab.watches.empty()) {
        ImGui::Text("watches:");
        ImGui::Separator();
        for (OpenSim::AbstractOutput const* ao : impl.outputs_tab.watches) {
            std::string v = ao->getValueAsString(impl.state);
            ImGui::Text("    %s/%s: %s", ao->getOwner().getName().c_str(), ao->getName().c_str(), v.c_str());
        }
    }

    // draw plots
    if (!impl.outputs_tab.plots.empty()) {
        ImGui::Text("plots:");
        ImGui::Separator();

        ImGui::Columns(2);
        for (Output_plot const& p : impl.outputs_tab.plots) {
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
*/

static void draw_selection_tab(Show_model_screen::Impl& impl) {
    if (!impl.selected_component) {
        ImGui::TextUnformatted("nothing selected, you can select things by:\n    - clicking something in the 3D viewer\n    - clicking something in the hierarchy browser");
        return;
    }

    SimTK::State& st = impl.active_state();

    // draw standard selection info
    {
        auto resp = ui::component_details::draw(st, impl.selected_component);

        if (resp.type == component_details::SelectionChanged) {
            impl.selected_component = resp.ptr;
        }
    }


        /* TODO
    // draw selection outputs (screen-specific)

    OpenSim::Component const& c = *impl.selected_component;

    // outputs


    if (ImGui::CollapsingHeader("outputs")) {
        size_t i = 0;
        for (auto const& ptr : c.getOutputs()) {
            OpenSim::AbstractOutput const* ao = ptr.second.get();

            ImGui::Columns(2);

            ImGui::TextUnformatted(ao->getName().c_str());
            ImGui::PushStyleColor(ImGuiCol_Text, 0xaaaaaaaa);
            ImGui::TextUnformatted(ao->getValueAsString(st).c_str());
            ImGui::PopStyleColor();
            ImGui::NextColumn();

            OpenSim::Output<double> const* od = dynamic_cast<OpenSim::Output<double> const*>(ao);
            if (od) {
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
                impl.selected_component.output_sinks[i++].draw(25.0f);

                if (ImGui::BeginPopupContextItem(od->getName().c_str())) {
                    if (ImGui::MenuItem("Add to outputs tab")) {
                        impl.outputs_tab.plots.emplace_back(ao);
                    }

                    ImGui::EndPopup();
                }
            }
            ImGui::NextColumn();

            ImGui::Columns();
            ImGui::Separator();
        }
    }
    */
}

static void draw_simulation_tab(Show_model_screen::Impl& impl) {
    if (ImGui::Button("Run")) {
        action_start_simulation(impl);
    }

    ImGui::TextUnformatted("mode:");
    ImGui::SameLine();    

    if (ImGui::Button("back to edited model")) {
        impl.select_model(-1);
    }

    for (size_t i = 0; i < impl.simulations.size(); ++i) {
        Ui_simulation& simulation = impl.simulations[i];

        ImGui::PushID(static_cast<int>(i));
        double cur = simulation.simulation.sim_current_time().count();
        double tot = simulation.simulation.sim_final_time().count();
        float pct = static_cast<float>(cur) / static_cast<float>(tot);

        if (ImGui::Button("select")) {
            impl.select_model(static_cast<int>(i));
        }
        
        ImGui::SameLine();
        ImGui::Text("%i", i);
        
        ImGui::SameLine();        
        if (ImGui::Button("x")) {
            if (impl.selected_idx >= static_cast<int>(i)) {
                impl.select_model(impl.selected_idx - 1);
            }

            impl.simulations.erase(impl.simulations.begin() + i);
        }

        ImGui::SameLine();
        ImGui::ProgressBar(pct);

        ImGui::PopID();
    }
}

// draw a frame of the UI
static void draw(Show_model_screen::Impl& impl) {

    // draw top menu bar
    if (ImGui::BeginMainMenuBar()) {
        // File tab
        ui::main_menu::file_tab::draw(impl.mm_filetab_st);

        // Actions tab
        if (ImGui::BeginMenu("Actions")) {
            if (ImGui::MenuItem("Start/Stop Simulation", "Space")) {
                action_start_simulation(impl);
            }

            if (ImGui::MenuItem("Reset Model to Initial State", "R")) {
                action_reset_model_to_initial_state(impl);
            }

            if (ImGui::MenuItem("Reload Model File", "Ctrl+R")) {
                action_try_reload_model_file(impl);
            }

            if (ImGui::MenuItem("Clear Selection", "Ctrl+A")) {
                action_clear_selection(impl);
            }

            if (ImGui::MenuItem("Switch to Editor", "Ctrl+E")) {
                action_switch_to_editor(impl);
            }

            ImGui::EndMenu();
        }

        // About tab
        ui::main_menu::about_tab::draw();

        if (ImGui::Button("Switch to editor (Ctrl+E)")) {
            auto copy = std::make_unique<OpenSim::Model>(*impl.edited_model);
            Application::current().request_screen_transition<Model_editor_screen>(std::move(copy));
        }

        ImGui::EndMainMenuBar();
    }

    // draw model viewer(s)
    for (size_t i = 0; i < impl.model_viewers.size(); ++i) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "viewer_%zu", i);

        auto& viewer = impl.model_viewers[i];

        auto resp = viewer.draw(
            buf, 
            impl.active_model(), 
            impl.active_state(), 
            impl.selected_component, 
            impl.hovered_component);

        if (resp.type == Response::Type::HoverChanged && viewer.is_moused_over()) {
            impl.hovered_component = resp.ptr;
        }

        if (resp.type == Response::Type::SelectionChanged && viewer.is_moused_over()) {
            impl.selected_component = resp.ptr;
        }
    }

    // only ever shows `edited` model
    if (ImGui::Begin("Hierarchy")) {
        OpenSim::Component const* selected = impl.selected_component;
        OpenSim::Component const* hovered = impl.hovered_component;

        // map selection/hover to `edited` model
        if (impl.selected_idx != -1) {
            selected = relocate_component_pointer_to_new_model(*impl.edited_model, selected);
            hovered = relocate_component_pointer_to_new_model(*impl.edited_model, hovered);
        }

        auto resp = component_hierarchy::draw(impl.edited_model.get(), selected, hovered);

        // map selection/hover from `edited` model
        if (resp.type == component_hierarchy::SelectionChanged) {
            if (impl.selected_idx != -1) {
                impl.selected_component = relocate_component_pointer_to_new_model(impl.active_model(), resp.ptr);
            } else {
                impl.selected_component = resp.ptr;
            }
        }

        if (resp.type == component_hierarchy::HoverChanged) {
            if (impl.selected_idx != -1) {
                impl.hovered_component = relocate_component_pointer_to_new_model(impl.active_model(), resp.ptr);
            } else {
                impl.hovered_component = resp.ptr;
            }
            
        }
    }
    ImGui::End();

    if (ImGui::Begin("Muscles")) {
        auto resp = muscles_table::draw(impl.muscles_table_st, impl.active_model(), impl.active_state());

        if (resp.type == muscles_table::Response::SelectionChanged) {
            impl.selected_component = resp.ptr;
        }

        if (resp.type == muscles_table::Response::HoverChanged) {
            impl.hovered_component = resp.ptr;
        }
    }
    ImGui::End();

    if (ImGui::Begin("Selection")) {
        draw_selection_tab(impl);
    }
    ImGui::End();

    if (ImGui::Begin("Coordinates")) {
        if (ui::coordinate_editor::draw(impl.coords_tab_st, *impl.edited_model, impl.edited_state)) {
            impl.edited_model->realizeReport(impl.edited_state);
            impl.select_model(-1);
        }
    }
    ImGui::End();

    if (ImGui::Begin("Simulation")) {
        draw_simulation_tab(impl);
    }
    ImGui::End();

    ui::log_viewer::draw(impl.log_viewer_st, "Log");
}

// public API

Show_model_screen::Show_model_screen(std::unique_ptr<OpenSim::Model> model) :
    impl{new Impl{std::move(model)}} {
}

Show_model_screen::~Show_model_screen() noexcept {
    delete impl;
}

bool Show_model_screen::on_event(SDL_Event const& e) {
    return handle_event(*impl, e);
}

void Show_model_screen::tick() {
    ::tick(*impl);
}

void Show_model_screen::draw() {
    ::draw(*impl);
}
