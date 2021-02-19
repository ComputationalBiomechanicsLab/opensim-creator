#include "show_model_screen.hpp"

#include "3d_common.hpp"
#include "algs.hpp"
#include "application.hpp"
#include "fd_simulation.hpp"
#include "hierarchy_viewer.hpp"
#include "loading_screen.hpp"
#include "opensim_wrapper.hpp"
#include "screen.hpp"
#include "sdl_wrapper.hpp"
#include "selection_viewer.hpp"
#include "simple_model_renderer.hpp"
#include "splash_screen.hpp"

#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Array.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Common/ComponentPath.h>
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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
    static void append_name(OpenSim::AbstractOutput const* ao, char* buf, size_t n) {
        std::snprintf(buf, n, "%s/%s", ao->getOwner().getName().c_str(), ao->getName().c_str());
    }

    static void get_coordinates(OpenSim::Model const& m, std::vector<OpenSim::Coordinate const*>& out) {
        OpenSim::CoordinateSet const& s = m.getCoordinateSet();
        int len = s.getSize();
        out.reserve(out.size() + static_cast<size_t>(len));
        for (int i = 0; i < len; ++i) {
            out.push_back(&s[i]);
        }
    }

    static void compute_moment_arms(
        OpenSim::Muscle const& muscle, SimTK::State const& st, OpenSim::Coordinate const& c, float* out, size_t steps) {

        SimTK::State state = st;
        muscle.getModel().realizeReport(state);

        bool prev_locked = c.getLocked(state);
        double prev_val = c.getValue(state);

        c.setLocked(state, false);

        double start = c.getRangeMin();
        double end = c.getRangeMax();
        double step = (end - start) / steps;

        for (size_t i = 0; i < steps; ++i) {
            double v = start + (i * step);
            c.setValue(state, v);
            out[i] = static_cast<float>(muscle.getGeometryPath().computeMomentArm(state, c));
        }

        c.setLocked(state, prev_locked);
        c.setValue(state, prev_val);
    }

    // holds a fixed number of Y datapoints that are assumed to be roughly evenly spaced in X
    //
    // if the number of datapoints "pushed" onto the sparkline exceeds the (fixed) capacity then
    // the datapoints will be halved (reducing resolution) to make room for more, which is how
    // it guarantees constant size
    template<size_t MaxDatapoints = 256>
    struct Evenly_spaced_sparkline final {
        static constexpr float min_x_step = 0.001f;
        static_assert(MaxDatapoints % 2 == 0, "num datapoints must be even because the impl uses integer division");

        std::array<float, MaxDatapoints> data;
        size_t n = 0;
        float x_step = min_x_step;
        float latest_x = -min_x_step;
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();

        constexpr Evenly_spaced_sparkline() = default;

        // reset the data, but not the output being monitored
        void clear() {
            n = 0;
            x_step = min_x_step;
            latest_x = -min_x_step;
            min = std::numeric_limits<float>::max();
            max = std::numeric_limits<float>::min();
        }

        void push_datapoint(float x, float y) {
            if (x < latest_x + x_step) {
                return;  // too close to previous datapoint: do not record
            }

            if (n == MaxDatapoints) {
                // too many datapoints recorded: half the resolution of the
                // sparkline to accomodate more datapoints being added
                size_t halfway = n / 2;
                for (size_t i = 0; i < halfway; ++i) {
                    size_t first = 2 * i;
                    data[i] = (data[first] + data[first + 1]) / 2.0f;
                }
                n = halfway;
                x_step *= 2.0f;
            }

            data[n++] = y;
            latest_x = x;
            min = std::min(min, y);
            max = std::max(max, y);
        }

        void draw(float height = 60.0f) const {
            ImGui::PlotLines(
                "",
                data.data(),
                static_cast<int>(n),
                0,
                nullptr,
                std::numeric_limits<float>::min(),
                std::numeric_limits<float>::max(),
                ImVec2(0, height));
        }

        float last_datapoint() const {
            assert(n > 0);
            return data[n - 1];
        }
    };

    struct Output_plot final {
        OpenSim::AbstractOutput const* ao;
        Evenly_spaced_sparkline<256> plot;

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
        using extrator_fn = float(osmv::Integrator_stats const&);

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

        void push_datapoint(float x, osmv::Integrator_stats const& stats) {
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
        std::optional<osmv::Fd_simulator> simulator;

        Evenly_spaced_sparkline<256> prescribeQcalls;
        Evenly_spaced_sparkline<256> simTimeDividedByWallTime;

        std::array<Integrator_stat_sparkline, 15> integrator_plots{
            {{"accuracyInUse", [](osmv::Integrator_stats const& is) { return static_cast<float>(is.accuracyInUse); }},
             {"predictedNextStepSize",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.predictedNextStepSize); }},
             {"numStepsAttempted",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numStepsAttempted); }},
             {"numStepsTaken", [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numStepsTaken); }},
             {"numRealizations",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numRealizations); }},
             {"numQProjections",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numQProjections); }},
             {"numUProjections",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numUProjections); }},
             {"numErrorTestFailures",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numErrorTestFailures); }},
             {"numConvergenceTestFailures",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numConvergenceTestFailures); }},
             {"numRealizationFailures",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numRealizationFailures); }},
             {"numQProjectionFailures",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numQProjectionFailures); }},
             {"numProjectionFailures",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numProjectionFailures); }},
             {"numConvergentIterations",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numConvergentIterations); }},
             {"numDivergentIterations",
              [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numDivergentIterations); }},
             {"numIterations", [](osmv::Integrator_stats const& is) { return static_cast<float>(is.numIterations); }}}};

        float fd_final_time = 0.4f;
        osmv::IntegratorMethod integrator_method = osmv::IntegratorMethod_OpenSimManagerDefault;

        osmv::Integrator_stats istats;

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

        void on_ui_state_update(OpenSim::Model const&, SimTK::State const& st) {
            if (not simulator) {
                return;
            }

            osmv::Fd_simulator const& sim = *simulator;
            float sim_time = static_cast<float>(st.getTime());
            float wall_time = std::chrono::duration<float>{sim.wall_duration()}.count();

            prescribeQcalls.push_datapoint(sim_time, static_cast<float>(sim.num_prescribeq_calls()));
            simTimeDividedByWallTime.push_datapoint(sim_time, sim_time / wall_time);

            // get latest integrator stats
            sim.integrator_stats(istats);

            // push 0d integrator stats onto sparklines
            for (Integrator_stat_sparkline& integrator_plot : integrator_plots) {
                integrator_plot.push_datapoint(sim_time, istats);
            }
        }

        void draw(
            osmv::Simple_model_renderer&, Selected_component&, OpenSim::Model& shown_model, SimTK::State& shown_state) {
            // start/stop button
            if (simulator and simulator->is_running()) {
                ImGui::PushStyleColor(ImGuiCol_Button, {1.0f, 0.0f, 0.0f, 1.0f});
                if (ImGui::Button("stop [SPC]")) {
                    simulator->request_stop();
                }
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.6f, 0.0f, 1.0f});
                if (ImGui::Button("start [SPC]")) {
                    osmv::Fd_simulation_params params{osmv::Model{shown_model},
                                                      osmv::State{shown_state},
                                                      static_cast<double>(fd_final_time),
                                                      integrator_method};
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
                        osmv::integrator_method_names,
                        osmv::IntegratorMethod_NumIntegratorMethods)) {
                    integrator_method = static_cast<osmv::IntegratorMethod>(method);
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
                ImGui::Text("simulator stats:");
                ImGui::Dummy(ImVec2{0.0f, 2.5f});
                ImGui::Separator();

                ImGui::Columns(2);
                ImGui::Text("status");
                ImGui::NextColumn();
                ImGui::Text("%s", simulator->status_description());
                ImGui::NextColumn();

                ImGui::Text("progress");
                ImGui::NextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::ProgressBar(static_cast<float>(frac_completed), ImVec2(0.0f, 0.0f));
                ImGui::NextColumn();

                ImGui::Text("simulation time");
                ImGui::NextColumn();
                ImGui::Text("%.2f s", sim_secs.count());
                ImGui::NextColumn();

                ImGui::Text("wall time");
                ImGui::NextColumn();
                ImGui::Text("%.2f s", wall_secs);
                ImGui::NextColumn();

                ImGui::Text("sim time / wall time (avg.)");
                ImGui::NextColumn();
                ImGui::Text("%.3f", (sim_secs / wall_secs).count());
                ImGui::NextColumn();

                ImGui::Text("`SimTK::State`s popped");
                ImGui::NextColumn();
                ImGui::Text("%i", simulator->num_states_popped());
                ImGui::NextColumn();

                ImGui::Text("UI overhead");
                ImGui::NextColumn();
                ImGui::Text("%.2f %%", 100.0 * simulator->avg_simulator_overhead());
                ImGui::NextColumn();

                ImGui::Columns();

                ImGui::Dummy({0.0f, 20.0f});
                ImGui::Text("plots:");
                ImGui::Dummy({0.0f, 2.5f});
                ImGui::Separator();

                ImGui::Columns(2);

                ImGui::Text("prescribeQcalls");
                ImGui::NextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                prescribeQcalls.draw(30.0f);
                ImGui::NextColumn();

                ImGui::Text("sim time / wall time");
                ImGui::NextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                simTimeDividedByWallTime.draw(30.0f);
                ImGui::NextColumn();

                for (Integrator_stat_sparkline& integrator_plot : integrator_plots) {
                    ImGui::Text("%s", integrator_plot.name);
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

    struct Muscles_tab_data final {
        // name filtering
        char filter[64]{};

        // length filtering
        float min_len = std::numeric_limits<float>::min();
        float max_len = std::numeric_limits<float>::max();
        bool inverse_range = false;

        // sorting
        static constexpr std::array<char const*, 2> sorting_choices{"length", "strain"};
        size_t current_sort_choice = 0;

        bool reverse_results = false;
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
                assert(o);
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

    using MuscleRecoloring = int;
    enum MuscleRecoloring_ {
        MuscleRecoloring_None = 0,
        MuscleRecoloring_Strain,
        MuscleRecoloring_Length,
        MuscleRecoloring_NumOptions,
    };
    std::array<char const*, MuscleRecoloring_NumOptions> muscle_coloring_options = {"none", "strain", "length"};
    static_assert(muscle_coloring_options.size() == MuscleRecoloring_NumOptions);
}

namespace osmv {
    struct Show_model_screen_impl final {
        // scratch: shared space that has no content guarantees
        struct {
            char text[1024];
            std::vector<OpenSim::Coordinate const*> coords;
            std::vector<OpenSim::Muscle const*> muscles;
        } scratch;

        std::filesystem::path model_path;
        osmv::Model model;
        osmv::State latest_state;

        Selected_component selected_component;

        Simple_model_renderer renderer;
        bool mouse_over_renderer = false;

        Coordinates_tab_data coords_tab;
        Simulator_tab simulator_tab;
        Momentarms_tab_data mas_tab;
        Muscles_tab_data muscles_tab;
        Outputs_tab_data outputs_tab;
        MuscleRecoloring muscle_recoloring = MuscleRecoloring_None;
        bool only_select_muscles = true;

        Show_model_screen_impl(Application& app, std::filesystem::path _path, osmv::Model _model) :
            model_path{std::move(_path)},
            model{std::move(_model)},
            latest_state{[this]() {
                model->finalizeFromProperties();
                osmv::State s{model->initSystem()};
                model->realizeReport(s);
                return s;
            }()},
            renderer{app.window_dimensions().w, app.window_dimensions().h, app.samples()} {

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
        }

        // handle top-level UI event (user click, user drag, etc.)
        bool handle_event(Application& app, SDL_Event const& e) {
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                case SDLK_r: {

                    // CTRL + R: reload the model from scratch
                    SDL_Keymod km = SDL_GetModState();
                    if (km & (KMOD_LCTRL | KMOD_RCTRL)) {
                        app.request_screen_transition<Loading_screen>(model_path);
                        return true;
                    }

                    // R: reset the model to its initial state
                    latest_state = model->initSystem();
                    on_user_edited_state();

                    return true;
                }
                case SDLK_SPACE: {
                    if (simulator_tab.simulator and simulator_tab.simulator->is_running()) {
                        simulator_tab.simulator->request_stop();
                    } else {
                        simulator_tab.simulator.emplace(
                            Fd_simulation_params{Model{model},
                                                 State{latest_state},
                                                 static_cast<double>(simulator_tab.fd_final_time),
                                                 simulator_tab.integrator_method});
                    }
                    break;
                case SDLK_ESCAPE:
                    app.request_screen_transition<Splash_screen>();
                    return true;
                case SDLK_c:
                    // clear selection
                    selected_component = nullptr;
                    return true;
                }
                }
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                // otherwise, maybe they're trying to select something in the viewport, so
                // check if they are hovered over a component and select it if they are
                if (e.button.button == SDL_BUTTON_RIGHT and renderer.hovered_component) {
                    selected_component = renderer.hovered_component;
                }
            }

            // if no events were captured above, let the model viewer handle
            // the event
            if (mouse_over_renderer or e.type == SDL_MOUSEBUTTONUP) {
                return renderer.on_event(e);
            }

            return false;
        }

        // "tick" the UI state (usually, used for updating animations etc.)
        void tick() {
            // grab the latest state (if any) from the simulator and (if updated)
            // update the UI to reflect the latest state
            if (simulator_tab.simulator and simulator_tab.simulator->try_pop_state(latest_state)) {
                model->realizeReport(latest_state);

                outputs_tab.on_ui_state_update(latest_state);
                simulator_tab.on_ui_state_update(model, latest_state);
                selected_component.on_ui_state_update(latest_state);
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
            return simulator_tab.simulator and simulator_tab.simulator->is_running();
        }

        // draw a frame of the UI
        void draw(Application& app) {

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
            if (ImGui::Begin("render")) {
                if (ImGui::BeginChild("child", ImVec2(0, 0), false, ImGuiWindowFlags_NoMove)) {
                    draw_render_tab();
                    ImGui::EndChild();
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();

            if (ImGui::Begin("Hierarchy")) {
                draw_hierarchy_tab();
            }
            ImGui::End();

            if (ImGui::Begin("Muscles")) {
                draw_muscles_tab();
            }
            ImGui::End();

            if (ImGui::Begin("Outputs")) {
                draw_outputs_tab();
            }
            ImGui::End();

            if (ImGui::Begin("Utils")) {
                draw_utils_tab();
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

            if (ImGui::Begin("UI")) {
                draw_ui_tab(app);
            }
            ImGui::End();

            if (ImGui::Begin("Coordinates")) {
                draw_coords_tab();
            }
            ImGui::End();

            if (ImGui::Begin("Simulate")) {
                draw_simulate_tab();
            }
            ImGui::End();
        }

        void draw_render_tab() {
            // generate OpenSim scene geometry
            renderer.generate_geometry(model, latest_state);

            // perform screen-specific geometry fixups
            if (only_select_muscles) {
                OpenSim::Model const* m = model.get();

                renderer.geometry.for_each([m](OpenSim::Component const*& associated_component, Mesh_instance const&) {
                    // for this screen specifically, the "owner"s should be fixed up to point to
                    // muscle objects, rather than direct (e.g. GeometryPath) objects
                    OpenSim::Component const* c = associated_component;
                    while (c != nullptr and c->hasOwner()) {
                        if (dynamic_cast<OpenSim::Muscle const*>(c)) {
                            break;
                        }
                        c = &c->getOwner();
                    }
                    if (c == m) {
                        c = nullptr;
                    }

                    associated_component = c;
                });
            }

            if (muscle_recoloring == MuscleRecoloring_Strain) {
                renderer.geometry.for_each([this](OpenSim::Component const* c, Mesh_instance& mi) {
                    OpenSim::Muscle const* musc = dynamic_cast<OpenSim::Muscle const*>(c);
                    if (not musc) {
                        return;
                    }

                    mi.rgba.r = static_cast<unsigned char>(255.0 * musc->getTendonStrain(latest_state));
                    mi.rgba.g = 255 / 2;
                    mi.rgba.b = 255 / 2;
                    mi.rgba.a = 255;
                });
            }

            if (muscle_recoloring == MuscleRecoloring_Length) {
                renderer.geometry.for_each([this](OpenSim::Component const* c, Mesh_instance& mi) {
                    OpenSim::Muscle const* musc = dynamic_cast<OpenSim::Muscle const*>(c);
                    if (not musc) {
                        return;
                    }

                    mi.rgba.r = static_cast<unsigned char>(255.0 * musc->getLength(latest_state));
                    mi.rgba.g = 255 / 4;
                    mi.rgba.b = 255 / 4;
                    mi.rgba.a = 255;
                });
            }

            // draw the scene to an OpenGL texture
            renderer.apply_standard_rim_coloring(selected_component);
            auto dims = ImGui::GetContentRegionAvail();
            renderer.reallocate_buffers(static_cast<int>(dims.x), static_cast<int>(dims.y), app().samples());

            gl::Texture_2d& render = renderer.draw();

            {
                // required by ImGui::Image
                //
                // UV coords: ImGui::Image uses different texture coordinates from the renderer
                //            (specifically, Y is reversed)
                void* texture_handle = reinterpret_cast<void*>(render.raw_handle());
                ImVec2 image_dimensions{dims.x, dims.y};
                ImVec2 uv0{0, 1};
                ImVec2 uv1{1, 0};

                ImVec2 cp = ImGui::GetCursorPos();
                ImVec2 mp = ImGui::GetMousePos();
                ImVec2 wp = ImGui::GetWindowPos();

                ImGui::Image(texture_handle, image_dimensions, uv0, uv1);

                mouse_over_renderer = ImGui::IsItemHovered();

                renderer.hovertest_x = static_cast<int>((mp.x - wp.x) - cp.x);
                // y is reversed (OpenGL coords, not screen)
                renderer.hovertest_y = static_cast<int>(dims.y - ((mp.y - wp.y) - cp.y));
            }

            // overlay: if the user is hovering over a component, write the component's name
            //          next to the mouse
            if (renderer.hovered_component) {
                OpenSim::Component const& c = *renderer.hovered_component;
                sdl::Mouse_state m = sdl::GetMouseState();
                ImVec2 pos{static_cast<float>(m.x + 20), static_cast<float>(m.y)};
                ImGui::GetBackgroundDrawList()->AddText(pos, 0xff0000ff, c.getName().c_str());
            }
        }

        void draw_ui_tab(Application& app) {
            ImGui::Text("%.1f fps", static_cast<double>(ImGui::GetIO().Framerate));

            // msxaa selector
            {
                static constexpr std::array<char const*, 8> aa_lvls = {
                    "x1", "x2", "x4", "x8", "x16", "x32", "x64", "x128"};
                int samples_idx = lsb_index(app.samples());
                int max_samples_idx = lsb_index(app.max_samples());
                assert(static_cast<size_t>(max_samples_idx) < aa_lvls.size());

                if (ImGui::Combo("samples", &samples_idx, aa_lvls.data(), max_samples_idx + 1)) {
                    app.set_samples(1 << samples_idx);
                }
            }

            ImGui::NewLine();

            ImGui::Text("Camera Position:");

            ImGui::NewLine();

            if (ImGui::Button("Front")) {
                // assumes models tend to point upwards in Y and forwards in +X
                renderer.theta = pi_f / 2.0f;
                renderer.phi = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Back")) {
                // assumes models tend to point upwards in Y and forwards in +X
                renderer.theta = 3.0f * (pi_f / 2.0f);
                renderer.phi = 0.0f;
            }

            ImGui::SameLine();
            ImGui::Text("|");
            ImGui::SameLine();

            if (ImGui::Button("Left")) {
                // assumes models tend to point upwards in Y and forwards in +X
                // (so sidewards is theta == 0 or PI)
                renderer.theta = pi_f;
                renderer.phi = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Right")) {
                // assumes models tend to point upwards in Y and forwards in +X
                // (so sidewards is theta == 0 or PI)
                renderer.theta = 0.0f;
                renderer.phi = 0.0f;
            }

            ImGui::SameLine();
            ImGui::Text("|");
            ImGui::SameLine();

            if (ImGui::Button("Top")) {
                renderer.theta = 0.0f;
                renderer.phi = pi_f / 2.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Bottom")) {
                renderer.theta = 0.0f;
                renderer.phi = 3.0f * (pi_f / 2.0f);
            }

            ImGui::NewLine();

            ImGui::SliderFloat("radius", &renderer.radius, 0.0f, 10.0f);
            ImGui::SliderFloat("theta", &renderer.theta, 0.0f, 2.0f * pi_f);
            ImGui::SliderFloat("phi", &renderer.phi, 0.0f, 2.0f * pi_f);
            ImGui::NewLine();
            ImGui::SliderFloat("pan_x", &renderer.pan.x, -100.0f, 100.0f);
            ImGui::SliderFloat("pan_y", &renderer.pan.y, -100.0f, 100.0f);
            ImGui::SliderFloat("pan_z", &renderer.pan.z, -100.0f, 100.0f);

            ImGui::NewLine();
            ImGui::Text("Lighting:");
            ImGui::SliderFloat("light_x", &renderer.light_pos.x, -30.0f, 30.0f);
            ImGui::SliderFloat("light_y", &renderer.light_pos.y, -30.0f, 30.0f);
            ImGui::SliderFloat("light_z", &renderer.light_pos.z, -30.0f, 30.0f);
            ImGui::ColorEdit3("light_color", reinterpret_cast<float*>(&renderer.light_rgb));
            ImGui::SliderFloat("rim thickness", &renderer.rim_thickness, 0.0f, 0.1f);
            {
                bool draw_rims = renderer.flags & SimpleModelRendererFlags_DrawRims;
                if (ImGui::Checkbox("draw rims", &draw_rims)) {
                    renderer.flags ^= SimpleModelRendererFlags_DrawRims;
                }
            }
            {
                bool show_floor = renderer.flags & SimpleModelRendererFlags_ShowFloor;
                if (ImGui::Checkbox("show_floor", &show_floor)) {
                    renderer.flags ^= SimpleModelRendererFlags_ShowFloor;
                }
            }
            {
                bool show_mesh_norms = renderer.flags & SimpleModelRendererFlags_ShowMeshNormals;
                if (ImGui::Checkbox("show_mesh_normals", &show_mesh_norms)) {
                    renderer.flags ^= SimpleModelRendererFlags_ShowMeshNormals;
                }
            }
            {
                bool hoverable_static_decs = renderer.flags & SimpleModelRendererFlags_HoverableStaticDecorations;
                if (ImGui::Checkbox("hoverable static geometry", &hoverable_static_decs)) {
                    renderer.flags ^= SimpleModelRendererFlags_HoverableStaticDecorations;
                }
            }
            {
                bool hoverable_dynamic_decs = renderer.flags & SimpleModelRendererFlags_HoverableDynamicDecorations;
                if (ImGui::Checkbox("hoverable dynamic geometry", &hoverable_dynamic_decs)) {
                    renderer.flags ^= SimpleModelRendererFlags_HoverableDynamicDecorations;
                }
            }
            ImGui::Checkbox("only select muscles", &only_select_muscles);

            // display hints
            {
                OpenSim::ModelDisplayHints& dh = model->updDisplayHints();

                {
                    bool debug_geom = dh.get_show_debug_geometry();
                    if (ImGui::Checkbox("show debug geometry", &debug_geom)) {
                        dh.set_show_debug_geometry(debug_geom);
                    }
                }

                {
                    bool frames_geom = dh.get_show_frames();
                    if (ImGui::Checkbox("show frames", &frames_geom)) {
                        dh.set_show_frames(frames_geom);
                    }
                }

                {
                    bool markers_geom = dh.get_show_markers();
                    if (ImGui::Checkbox("show markers", &markers_geom)) {
                        dh.set_show_markers(markers_geom);
                    }
                }
            }

            {
                ImGui::Combo(
                    "muscle coloring",
                    &muscle_recoloring,
                    muscle_coloring_options.data(),
                    static_cast<int>(muscle_coloring_options.size()));
            }

            if (ImGui::Button("fullscreen")) {
                app.make_fullscreen();
            }

            if (ImGui::Button("windowed")) {
                app.make_windowed();
            }

            if (not app.is_vsync_enabled()) {
                if (ImGui::Button("enable vsync")) {
                    app.enable_vsync();
                }
            } else {
                if (ImGui::Button("disable vsync")) {
                    app.disable_vsync();
                }
            }

            ImGui::NewLine();
            ImGui::Text("Interaction: ");
            if (renderer.flags & SimpleModelRendererFlags_Dragging) {
                ImGui::SameLine();
                ImGui::Text("rotating ");
            }
            if (renderer.flags & SimpleModelRendererFlags_Panning) {
                ImGui::SameLine();
                ImGui::Text("panning ");
            }
            if (mouse_over_renderer) {
                ImGui::SameLine();
                ImGui::Text("interacting ");
            }
        }

        void draw_simulate_tab() {
            simulator_tab.draw(renderer, selected_component, model, latest_state);
        }

        void draw_coords_tab() {
            // render coordinate filters
            {
                ImGui::Text("filters:");
                ImGui::Dummy({0.0f, 2.5f});
                ImGui::Separator();

                ImGui::Columns(2);

                ImGui::Text("search");
                ImGui::NextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##coords search filter", coords_tab.filter, sizeof(coords_tab.filter));
                ImGui::NextColumn();

                ImGui::Text("sort alphabetically");
                ImGui::NextColumn();
                ImGui::Checkbox("##coords alphabetical sort", &coords_tab.sort_by_name);
                ImGui::NextColumn();

                ImGui::Text("show rotational");
                ImGui::NextColumn();
                ImGui::Checkbox("##rotational coordinates checkbox", &coords_tab.show_rotational);
                ImGui::NextColumn();

                ImGui::Text("show translational");
                ImGui::NextColumn();
                ImGui::Checkbox("##translational coordinates checkbox", &coords_tab.show_translational);
                ImGui::NextColumn();

                ImGui::Text("show coupled");
                ImGui::NextColumn();
                ImGui::Checkbox("##coupled coordinates checkbox", &coords_tab.show_coupled);
                ImGui::NextColumn();

                ImGui::Columns();
            }

            // load coords
            scratch.coords.clear();
            get_coordinates(model, scratch.coords);

            // sort coords
            {
                auto should_remove = [&](OpenSim::Coordinate const* c) {
                    if (c->getName().find(coords_tab.filter) == c->getName().npos) {
                        return true;
                    }

                    OpenSim::Coordinate::MotionType mt = c->getMotionType();
                    if (coords_tab.show_rotational and mt == OpenSim::Coordinate::MotionType::Rotational) {
                        return false;
                    }

                    if (coords_tab.show_translational and mt == OpenSim::Coordinate::MotionType::Translational) {
                        return false;
                    }

                    if (coords_tab.show_coupled and mt == OpenSim::Coordinate::MotionType::Coupled) {
                        return false;
                    }

                    return true;
                };

                auto it = std::remove_if(scratch.coords.begin(), scratch.coords.end(), should_remove);
                scratch.coords.erase(it, scratch.coords.end());
            }

            // sort coords
            if (coords_tab.sort_by_name) {
                auto by_name = [](OpenSim::Coordinate const* c1, OpenSim::Coordinate const* c2) {
                    return c1->getName() < c2->getName();
                };

                std::sort(scratch.coords.begin(), scratch.coords.end(), by_name);
            }

            // render coordinates list
            ImGui::Dummy({0.0f, 10.0f});
            ImGui::Text("coordinates (%zu):", scratch.coords.size());
            ImGui::Dummy({0.0f, 2.5f});
            ImGui::Separator();

            ImGui::Columns(2);
            int i = 0;
            for (OpenSim::Coordinate const* c : scratch.coords) {
                ImGui::PushID(i++);

                ImGui::Text("%s", c->getName().c_str());
                ImGui::NextColumn();

                // if locked, color everything red
                int styles_pushed = 0;
                if (c->getLocked(latest_state)) {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.6f, 0.0f, 0.0f, 1.0f});
                    ++styles_pushed;
                }

                if (ImGui::Button(c->getLocked(latest_state) ? "u" : "l")) {
                    c->setLocked(latest_state, not c->getLocked(latest_state));
                    on_user_edited_state();
                }

                ImGui::SameLine();

                float v = static_cast<float>(c->getValue(latest_state));
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::SliderFloat(
                        " ", &v, static_cast<float>(c->getRangeMin()), static_cast<float>(c->getRangeMax()))) {
                    c->setValue(latest_state, static_cast<double>(v));
                    on_user_edited_state();
                }

                ImGui::PopStyleColor(styles_pushed);
                ImGui::NextColumn();

                ImGui::PopID();
            }
            ImGui::Columns();
        }

        void draw_utils_tab() {
            // tab containing one-off utilities that are useful when diagnosing a model

            ImGui::Text("wrapping surfaces: ");
            ImGui::SameLine();
            if (ImGui::Button("disable")) {
                OpenSim::Model& m = model;
                for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
                    for (int i = 0; i < wos.getSize(); ++i) {
                        OpenSim::WrapObject& wo = wos[i];
                        wo.set_active(false);
                        wo.upd_Appearance().set_visible(false);
                    }
                }
                on_user_edited_model();
            }
            ImGui::SameLine();
            if (ImGui::Button("enable")) {
                OpenSim::Model& m = model;
                for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
                    for (int i = 0; i < wos.getSize(); ++i) {
                        OpenSim::WrapObject& wo = wos[i];
                        wo.set_active(true);
                        wo.upd_Appearance().set_visible(true);
                    }
                }
                on_user_edited_model();
            }

            ImGui::Text("tendon strain");
            ImGui::SameLine();
        }

        void draw_hierarchy_tab() {
            Hierarchy_viewer v;
            OpenSim::Component const* selected = selected_component.get();
            v.draw(&model->getRoot(), &selected, &renderer.hovered_component);
            selected_component = selected;
        }

        void draw_muscles_tab() {
            // extract muscles details from model
            scratch.muscles.clear();
            for (OpenSim::Muscle const& musc : model->getComponentList<OpenSim::Muscle>()) {
                scratch.muscles.push_back(&musc);
            }

            ImGui::Text("filters:");
            ImGui::Dummy({0.0f, 2.5f});
            ImGui::Separator();

            ImGui::Columns(2);

            ImGui::Text("search");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputText("##muscles search filter", muscles_tab.filter, sizeof(muscles_tab.filter));
            ImGui::NextColumn();

            ImGui::Text("min length");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputFloat("##muscles min filter", &muscles_tab.min_len);
            ImGui::NextColumn();

            ImGui::Text("max length");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputFloat("##muscles max filter", &muscles_tab.max_len);
            ImGui::NextColumn();

            ImGui::Text("inverse length range");
            ImGui::NextColumn();
            ImGui::Checkbox("##muscles inverse range filter", &muscles_tab.inverse_range);
            ImGui::NextColumn();

            ImGui::Text("sort by");
            ImGui::NextColumn();
            ImGui::PushID("muscles sort by checkbox");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::BeginCombo(
                    " ", muscles_tab.sorting_choices[muscles_tab.current_sort_choice], ImGuiComboFlags_None)) {
                for (size_t n = 0; n < muscles_tab.sorting_choices.size(); n++) {
                    bool is_selected = (muscles_tab.current_sort_choice == n);
                    if (ImGui::Selectable(muscles_tab.sorting_choices[n], is_selected)) {
                        muscles_tab.current_sort_choice = n;
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::Text("reverse results");
            ImGui::NextColumn();
            ImGui::Checkbox("##muscles reverse results chechbox", &muscles_tab.reverse_results);
            ImGui::NextColumn();

            ImGui::Columns();

            // all user filters handled, transform the muscle list accordingly.

            // filter muscle list
            {
                auto filter_fn = [&](OpenSim::Muscle const* m) {
                    bool in_range = muscles_tab.min_len <= static_cast<float>(m->getLength(latest_state)) and
                                    static_cast<float>(m->getLength(latest_state)) <= muscles_tab.max_len;

                    if (muscles_tab.inverse_range) {
                        in_range = not in_range;
                    }

                    if (not in_range) {
                        return true;
                    }

                    bool matches_filter = m->getName().find(muscles_tab.filter) != m->getName().npos;

                    return not matches_filter;
                };

                auto it = std::remove_if(scratch.muscles.begin(), scratch.muscles.end(), filter_fn);
                scratch.muscles.erase(it, scratch.muscles.end());
            }

            // sort muscle list
            assert(not muscles_tab.sorting_choices.empty());
            assert(muscles_tab.current_sort_choice < muscles_tab.sorting_choices.size());
            switch (muscles_tab.current_sort_choice) {
            case 0: {  // sort muscles by length
                std::sort(
                    scratch.muscles.begin(),
                    scratch.muscles.end(),
                    [this](OpenSim::Muscle const* m1, OpenSim::Muscle const* m2) {
                        return m1->getLength(latest_state) > m2->getLength(latest_state);
                    });
                break;
            }
            case 1: {  // sort muscles by tendon strain
                std::sort(
                    scratch.muscles.begin(),
                    scratch.muscles.end(),
                    [&](OpenSim::Muscle const* m1, OpenSim::Muscle const* m2) {
                        return m1->getTendonStrain(latest_state) > m2->getTendonStrain(latest_state);
                    });
                break;
            }
            default:
                break;  // skip sorting
            }

            // reverse list (if necessary)
            if (muscles_tab.reverse_results) {
                std::reverse(scratch.muscles.begin(), scratch.muscles.end());
            }

            ImGui::Dummy({0.0f, 20.0f});
            ImGui::Text("results (%zu):", scratch.muscles.size());
            ImGui::Dummy({0.0f, 2.5f});
            ImGui::Separator();

            // muscle table header
            ImGui::Columns(4);
            ImGui::Text("name");
            ImGui::NextColumn();
            ImGui::Text("length");
            ImGui::NextColumn();
            ImGui::Text("strain");
            ImGui::NextColumn();
            ImGui::Text("force");
            ImGui::NextColumn();
            ImGui::Columns();
            ImGui::Separator();

            // muscle table rows
            ImGui::Columns(4);
            for (OpenSim::Muscle const* musc : scratch.muscles) {
                ImGui::Text("%s", musc->getName().c_str());
                if (ImGui::IsItemHovered()) {
                    renderer.hovered_component = musc;
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    selected_component = musc;
                }
                ImGui::NextColumn();
                ImGui::Text("%.3f", static_cast<double>(musc->getLength(latest_state)));
                ImGui::NextColumn();
                ImGui::Text("%.3f", 100.0 * musc->getTendonStrain(latest_state));
                ImGui::NextColumn();
                ImGui::Text("%.3f", musc->getTendonForce(latest_state));
                ImGui::NextColumn();
            }
            ImGui::Columns();
        }

        void draw_moment_arms_tab() {
            ImGui::Columns(2);

            // lhs: muscle selection
            {
                ImGui::Text("muscles:");
                ImGui::Dummy({0.0f, 5.0f});

                scratch.muscles.clear();
                for (OpenSim::Muscle const& musc : model->getComponentList<OpenSim::Muscle>()) {
                    scratch.muscles.push_back(&musc);
                }

                // usability: sort by name
                std::sort(
                    scratch.muscles.begin(),
                    scratch.muscles.end(),
                    [](OpenSim::Muscle const* m1, OpenSim::Muscle const* m2) { return m1->getName() < m2->getName(); });

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
                ImGui::BeginChild(
                    "MomentArmPlotMuscleSelection", ImVec2(ImGui::GetContentRegionAvail().x, 260), false, window_flags);
                for (OpenSim::Muscle const* m : scratch.muscles) {
                    if (ImGui::Selectable(m->getName().c_str(), &m->getName() == mas_tab.selected_musc)) {
                        mas_tab.selected_musc = &m->getName();
                    }
                }
                ImGui::EndChild();
                ImGui::NextColumn();
            }
            // rhs: coord selection
            {
                ImGui::Text("coordinates:");
                ImGui::Dummy({0.0f, 5.0f});

                scratch.coords.clear();
                get_coordinates(model, scratch.coords);

                // usability: sort by name
                std::sort(
                    scratch.coords.begin(),
                    scratch.coords.end(),
                    [](OpenSim::Coordinate const* c1, OpenSim::Coordinate const* c2) {
                        return c1->getName() < c2->getName();
                    });

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
                ImGui::BeginChild(
                    "MomentArmPlotCoordSelection", ImVec2(ImGui::GetContentRegionAvail().x, 260), false, window_flags);
                for (OpenSim::Coordinate const* c : scratch.coords) {
                    if (ImGui::Selectable(c->getName().c_str(), &c->getName() == mas_tab.selected_coord)) {
                        mas_tab.selected_coord = &c->getName();
                    }
                }
                ImGui::EndChild();
                ImGui::NextColumn();
            }
            ImGui::Columns();

            if (mas_tab.selected_musc and mas_tab.selected_coord) {
                if (ImGui::Button("+ add plot")) {
                    auto it =
                        std::find_if(scratch.muscles.begin(), scratch.muscles.end(), [this](OpenSim::Muscle const* ms) {
                            return &ms->getName() == mas_tab.selected_musc;
                        });
                    assert(it != scratch.muscles.end());

                    auto it2 = std::find_if(
                        scratch.coords.begin(), scratch.coords.end(), [this](OpenSim::Coordinate const* c) {
                            return &c->getName() == mas_tab.selected_coord;
                        });
                    assert(it2 != scratch.coords.end());

                    auto p = std::make_unique<Moment_arm_plot>();
                    p->muscle_name = *mas_tab.selected_musc;
                    p->coord_name = *mas_tab.selected_coord;
                    p->x_begin = static_cast<float>((*it2)->getRangeMin());
                    p->x_end = static_cast<float>((*it2)->getRangeMax());

                    // populate y values
                    compute_moment_arms(**it, latest_state, **it2, p->y_vals.data(), p->y_vals.size());
                    float min = std::numeric_limits<float>::max();
                    float max = std::numeric_limits<float>::min();
                    for (float v : p->y_vals) {
                        min = std::min(min, v);
                        max = std::max(max, v);
                    }
                    p->min = min;
                    p->max = max;

                    // clear current coordinate selection to prevent user from double
                    // clicking plot by accident *but* don't clear muscle because it's
                    // feasible that a user will want to plot other coords against the
                    // same muscle
                    mas_tab.selected_coord = nullptr;

                    mas_tab.plots.push_back(std::move(p));
                }
            }

            if (ImGui::Button("refresh TODO")) {
                // iterate through each plot in plots vector and recompute the moment
                // arms from the UI's current model + state
                throw std::runtime_error{"refreshing moment arm plots NYI"};
            }

            if (not mas_tab.plots.empty() and ImGui::Button("clear all")) {
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

            // apply user filters
            {
                auto it = std::remove_if(
                    outputs_tab.available.begin(), outputs_tab.available.end(), [&](OpenSim::AbstractOutput const* ao) {
                        append_name(ao, scratch.text, sizeof(scratch.text));
                        return std::strstr(scratch.text, outputs_tab.filter) == nullptr;
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
                        scratch.text,
                        sizeof(scratch.text),
                        "%s/%s",
                        ao->getOwner().getName().c_str(),
                        ao->getName().c_str());
                    if (ImGui::Selectable(scratch.text, ao == outputs_tab.selected)) {
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
            if (not outputs_tab.watches.empty()) {
                ImGui::Text("watches:");
                ImGui::Separator();
                for (OpenSim::AbstractOutput const* ao : outputs_tab.watches) {
                    std::string v = ao->getValueAsString(latest_state);
                    ImGui::Text("    %s/%s: %s", ao->getOwner().getName().c_str(), ao->getName().c_str(), v.c_str());
                }
            }

            // draw plots
            if (not outputs_tab.plots.empty()) {
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
            if (not selected_component) {
                ImGui::Text("nothing selected: right click a muscle");
                return;
            }

            // draw standard selection info
            {
                OpenSim::Component const* c = selected_component.get();
                Selection_viewer{}.draw(latest_state, &c);
                selected_component = c;
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
}

// screen PIMPL forwarding

osmv::Show_model_screen::Show_model_screen(std::filesystem::path path, osmv::Model model) :
    impl{new Show_model_screen_impl{app(), std::move(path), std::move(model)}} {
}
osmv::Show_model_screen::~Show_model_screen() noexcept {
    delete impl;
}

bool osmv::Show_model_screen::on_event(SDL_Event const& e) {
    return impl->handle_event(app(), e);
}

void osmv::Show_model_screen::tick() {
    return impl->tick();
}

void osmv::Show_model_screen::draw() {
    impl->draw(app());
}
