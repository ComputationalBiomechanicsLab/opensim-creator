#include "show_model_screen.hpp"

#include "osmv_config.hpp"

#include "3d_common.hpp"
#include "application.hpp"
#include "cfg.hpp"
#include "fd_simulation.hpp"
#include "gl.hpp"
#include "loading_screen.hpp"
#include "opensim_wrapper.hpp"
#include "os.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "splash_screen.hpp"

#include "imgui.h"
#include <OpenSim/Actuators/Millard2012EquilibriumMuscle.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    static void append_name(OpenSim::AbstractOutput const* ao, char* buf, size_t n) {
        std::snprintf(buf, n, "%s/%s", ao->getOwner().getName().c_str(), ao->getName().c_str());
    }

    static bool produces_doubles(OpenSim::AbstractOutput const* ao) noexcept {
        return (not ao->isListOutput()) and dynamic_cast<OpenSim::Output<double> const*>(ao) != nullptr;
    }

    // flag-ified version of OpenSim::Coordinate::MotionType (easier ORing for filtering)
    enum Motion_type : int {
        Undefined = 0,
        Rotational = 1,
        Translational = 2,
        Coupled = 4,
    };
    static_assert(Motion_type::Undefined == 0);

    // info for a coordinate in a model
    //
    // pointers in this struct are dependent on the model: only use this in short-lived contexts
    // and don't let it survive during a model edit or model destruction
    struct Coordinate final {
        OpenSim::Coordinate const* ptr;
        std::string const* name;
        float min;
        float max;
        float value;
        Motion_type type;
        bool locked;
    };

    // info for a muscle in a model
    //
    // pointers in this struct are dependent on the model: only use this in short-lived contexts
    // and don't let it survive during a model edit or model destruction
    struct Muscle_stat final {
        OpenSim::Muscle const* ptr;
        std::string const* name;
        float length;
    };

    static Motion_type convert_to_osim_motiontype(OpenSim::Coordinate::MotionType m) {
        using OpenSim::Coordinate;

        switch (m) {
        case Coordinate::MotionType::Undefined:
            return Motion_type::Undefined;
        case Coordinate::MotionType::Rotational:
            return Motion_type::Rotational;
        case Coordinate::MotionType::Translational:
            return Motion_type::Translational;
        case Coordinate::MotionType::Coupled:
            return Motion_type::Coupled;
        default:
            throw std::runtime_error{"convert_to_osim_motiontype: unknown coordinate type encountered"};
        }
    }

    static void get_coordinates(OpenSim::Model const& m, SimTK::State const& st, std::vector<Coordinate>& out) {
        OpenSim::CoordinateSet const& s = m.getCoordinateSet();
        int len = s.getSize();
        out.reserve(out.size() + static_cast<size_t>(len));
        for (int i = 0; i < len; ++i) {
            OpenSim::Coordinate const& c = s[i];
            out.push_back(Coordinate{
                &c,
                &c.getName(),
                static_cast<float>(c.getRangeMin()),
                static_cast<float>(c.getRangeMax()),
                static_cast<float>(c.getValue(st)),
                convert_to_osim_motiontype(c.getMotionType()),
                c.getLocked(st),
            });
        }
    }

    static void get_muscle_stats(OpenSim::Model const& m, SimTK::State const& s, std::vector<Muscle_stat>& out) {
        for (OpenSim::Muscle const& musc : m.getComponentList<OpenSim::Muscle>()) {
            out.push_back(Muscle_stat{
                &musc,
                &musc.getName(),
                static_cast<float>(musc.getLength(s)),
            });
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

    static const ImVec4 red{1.0f, 0.0f, 0.0f, 1.0f};
    static const ImVec4 dark_green{0.0f, 0.6f, 0.0f, 1.0f};
    static const ImVec4 dark_red{0.6f, 0.0f, 0.0f, 1.0f};

    // holds a fixed number of Y datapoints that are assumed to be roughly evenly spaced in X
    //
    // if the number of datapoints "pushed" onto the sparkline exceeds the (fixed) capacity then
    // the datapoints will be halved (reducing resolution) to make room for more, which is how
    // it guarantees constant size
    template<size_t MaxDatapoints = 256>
    struct Evenly_spaced_sparkline final {
        static constexpr float min_x_step = 0.005f;
        static_assert(MaxDatapoints % 2 == 0, "num datapoints must be even because the impl uses integer division");

        std::array<float, MaxDatapoints> data;
        size_t n = 0;
        float x_step = min_x_step;
        float latest_x = -min_x_step;
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();

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

        void draw(float height = 100.0f) const {
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

    struct Simulator_tab_data final {
        Evenly_spaced_sparkline<256> prescribeQcalls;
        Evenly_spaced_sparkline<256> simTimeDividedByWallTime;
        Evenly_spaced_sparkline<256> numIntegrationSteps;
        Evenly_spaced_sparkline<256> integrationStepsAttempted;
        Evenly_spaced_sparkline<256> stepsOverAttempts;

        float fd_final_time = 0.4f;

        void clear() {
            prescribeQcalls.clear();
            simTimeDividedByWallTime.clear();
            numIntegrationSteps.clear();
            integrationStepsAttempted.clear();
        }

        void on_user_edited_model() {
            clear();
        }

        void on_user_edited_state() {
            clear();
        }

        void on_ui_state_update(osmv::Fd_simulator const& sim, SimTK::State const& st) {
            float sim_time = static_cast<float>(st.getTime());
            float wall_time = std::chrono::duration<float>{sim.wall_duration()}.count();
            float num_integration_steps = static_cast<float>(sim.num_integration_steps());
            float num_step_attempts = static_cast<float>(sim.num_integration_step_attempts());

            prescribeQcalls.push_datapoint(sim_time, sim.num_prescribeq_calls());
            simTimeDividedByWallTime.push_datapoint(sim_time, sim_time / wall_time);
            numIntegrationSteps.push_datapoint(sim_time, num_integration_steps);
            integrationStepsAttempted.push_datapoint(sim_time, num_step_attempts);
            stepsOverAttempts.push_datapoint(sim_time, num_integration_steps / num_step_attempts);
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
}

namespace osmv {
    struct Show_model_screen_impl final {
        // scratch: shared space that has no content guarantees
        struct {
            char text[1024 + 1];
            std::vector<Coordinate> coords;
            std::vector<Muscle_stat> muscles;
        } scratch;

        // model
        std::filesystem::path path;
        osmv::Model model;
        osmv::State latest_state;

        Renderer renderer;
        std::optional<Fd_simulator> simulator;

        Coordinates_tab_data t_coords;
        Simulator_tab_data t_simulator;
        Momentarms_tab_data t_momentarms;
        Muscles_tab_data t_muscs;
        Outputs_tab_data t_outputs;

        Show_model_screen_impl(std::filesystem::path _path, osmv::Model _model) :
            path{std::move(_path)},
            model{std::move(_model)},
            latest_state{[this]() {
                model->finalizeFromProperties();
                osmv::State s{model->initSystem()};
                model->realizeReport(s);
                return s;
            }()} {
        }

        // handle top-level UI event (user click, user drag, etc.)
        Event_response handle_event(Application& app, SDL_Event const& e) {
            ImGuiIO& io = ImGui::GetIO();
            sdl::Window_dimensions window_dims = app.window_dimensions();

            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                case SDLK_r: {
                    auto km = SDL_GetModState();
                    if (km & (KMOD_LCTRL | KMOD_RCTRL)) {
                        app.request_screen_transition<Loading_screen>(path);
                        return Event_response::handled;
                    } else {
                        latest_state = model->initSystem();
                        on_user_edited_state();
                    }
                    break;
                }
                case SDLK_SPACE: {
                    if (simulator and simulator->is_running()) {
                        simulator->request_stop();
                    } else {
                        simulator.emplace(
                            Fd_simulation_params{Model{model}, State{latest_state}, t_simulator.fd_final_time});
                    }
                    break;
                case SDLK_ESCAPE:
                    app.request_screen_transition<Splash_screen>();
                    return Event_response::handled;
                }
                }
            } else if (e.type == SDL_MOUSEMOTION) {
                if (io.WantCaptureMouse) {
                    // if ImGUI wants to capture the mouse, then the mouse
                    // is probably interacting with an ImGUI panel and,
                    // therefore, the dragging/panning shouldn't be handled
                    return Event_response::ignored;
                }
            } else if (e.type == SDL_WINDOWEVENT) {
                window_dims = app.window_dimensions();
                glViewport(0, 0, window_dims.w, window_dims.h);
            } else if (e.type == SDL_MOUSEWHEEL) {
                if (io.WantCaptureMouse) {
                    // if ImGUI wants to capture the mouse, then the mouse
                    // is probably interacting with an ImGUI panel and,
                    // therefore, the dragging/panning shouldn't be handled
                    return Event_response::ignored;
                }
            }

            // if no events were captured above, let the model viewer handle
            // the event
            return renderer.on_event(app, e);
        }

        // "tick" the UI state (usually, used for updating animations etc.)
        void tick() {
            // grab the latest state (if any) from the simulator and (if updated)
            // update the UI to reflect the latest state
            if (simulator and simulator->try_pop_latest_state(latest_state)) {
                model->realizeReport(latest_state);

                t_outputs.on_ui_state_update(latest_state);
                t_simulator.on_ui_state_update(*simulator, latest_state);
            }
        }

        void on_user_edited_model() {
            // these might be invalidated by changing the model because they might
            // contain (e.g.) pointers into the model
            simulator = std::nullopt;
            t_momentarms.selected_musc = nullptr;
            t_momentarms.selected_coord = nullptr;

            t_outputs.on_user_edited_model();
            t_simulator.on_user_edited_model();

            latest_state = model->initSystem();
            model->realizeReport(latest_state);
        }

        void on_user_edited_state() {
            // stop the simulator whenever a user-initiated state change happens
            if (simulator) {
                simulator->request_stop();
            }

            model->realizeReport(latest_state);

            t_outputs.on_user_edited_state();
            t_simulator.on_user_edited_state();
        }

        bool simulator_running() {
            return simulator and simulator->is_running();
        }

        // draw a frame of the UI
        void draw(Application& app) {

            // draw OpenSim 3D model using renderer
            renderer.draw(app, model, latest_state);

            // draw ImGui panels on top of 3D scene
            draw_menu_bar();
            draw_ui_panel(app);
            draw_lhs_panel();
        }

        void draw_menu_bar() {
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginTabBar("MainTabBar")) {
                    if (ImGui::BeginTabItem(path.filename().string().c_str())) {
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::EndMainMenuBar();
            }
        }

        void draw_ui_panel(Application& app) {
            bool b = true;
            ImGuiWindowFlags flags = ImGuiWindowFlags_Modal;

            if (ImGui::Begin("UI", &b, flags)) {
                draw_ui_tab(app);
            }
            ImGui::End();
        }

        void draw_ui_tab(Application& app) {
            ImGui::Text("Fps: %.1f", static_cast<double>(ImGui::GetIO().Framerate));
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
            ImGui::ColorEdit3("light_color", reinterpret_cast<float*>(&renderer.light_color));
            ImGui::Checkbox("show_floor", &renderer.show_floor);
            ImGui::Checkbox("gamma_correction", &renderer.gamma_correction);
            {
                bool throttling = app.is_throttling_fps();
                if (ImGui::Checkbox("fps_throttle", &throttling)) {
                    app.is_throttling_fps(throttling);
                }
            }
            ImGui::Checkbox("show_mesh_normals", &renderer.show_mesh_normals);

            ImGui::NewLine();
            ImGui::Text("Interaction: ");
            if (renderer.dragging) {
                ImGui::SameLine();
                ImGui::Text("rotating ");
            }
            if (renderer.panning) {
                ImGui::SameLine();
                ImGui::Text("panning ");
            }
        }

        void draw_lhs_panel() {
            bool b = true;
            ImGuiWindowFlags flags = ImGuiWindowFlags_Modal;

            if (ImGui::Begin("Model", &b, flags)) {
                if (ImGui::BeginTabBar("SomeTabBar")) {

                    if (ImGui::BeginTabItem("Outputs")) {
                        ImGui::Dummy(ImVec2{0.0f, 5.0f});
                        draw_outputs_tab();
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Simulate")) {
                        ImGui::Dummy(ImVec2{0.0f, 5.0f});
                        draw_simulate_tab();
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Muscles")) {
                        ImGui::Dummy(ImVec2{0.0f, 5.0f});
                        draw_muscles_tab();
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Coords")) {
                        ImGui::Dummy(ImVec2{0.0f, 5.0f});
                        draw_coords_tab();
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Utils")) {
                        ImGui::Dummy(ImVec2{0.0f, 5.0f});
                        draw_utils_tab();
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("MAs")) {
                        ImGui::Dummy(ImVec2{0.0f, 5.0f});
                        draw_moment_arms_tab();
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
            }
            ImGui::End();
        }

        void draw_simulate_tab() {
            // start/stop button
            if (simulator_running()) {
                ImGui::PushStyleColor(ImGuiCol_Button, red);
                if (ImGui::Button("stop [SPC]")) {
                    simulator->request_stop();
                }
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, dark_green);
                if (ImGui::Button("start [SPC]")) {
                    Fd_simulation_params params{Model{model}, State{latest_state}, t_simulator.fd_final_time};
                    simulator.emplace(std::move(params));
                }
                ImGui::PopStyleColor();
            }

            ImGui::SameLine();
            if (ImGui::Button("reset [r]")) {
                latest_state = model->initSystem();
                on_user_edited_state();
            }

            ImGui::Dummy(ImVec2{0.0f, 20.0f});

            ImGui::Text("simulation config");
            ImGui::Separator();
            ImGui::SliderFloat("final time", &t_simulator.fd_final_time, 0.01f, 20.0f);

            if (simulator) {
                std::chrono::milliseconds wall_ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(simulator->wall_duration());
                double wall_secs = static_cast<double>(wall_ms.count()) / 1000.0;
                std::chrono::duration<double> sim_secs = simulator->sim_current_time();
                double pct_completed = sim_secs / simulator->sim_final_time() * 100.0;

                ImGui::Dummy(ImVec2{0.0f, 20.0f});
                ImGui::Text("simulator stats");
                ImGui::Separator();
                ImGui::Text("status: %s", simulator->status_description());
                ImGui::Text("progress: %.1f %%", pct_completed);
                ImGui::Text("simulation time: %.2f", sim_secs.count());
                ImGui::Text("wall time: %.2f secs", wall_secs);
                ImGui::Text("avg. sim_time/wall_time: %.4f", (sim_secs / wall_secs).count());
                ImGui::Text("total prescribeQ calls: %i", simulator->num_prescribeq_calls());
                ImGui::Text("total SimTK::States copied to UI thread: %i", simulator->num_states_popped());
                ImGui::Text("avg. UI overhead in sim thread %.2f %%", 100.0 * simulator->avg_ui_overhead_pct());

                ImGui::Separator();

                ImGui::Text("prescribeQcalls");
                t_simulator.prescribeQcalls.draw(50.0f);

                ImGui::Text("sim time / wall time");
                t_simulator.simTimeDividedByWallTime.draw(50.0f);

                ImGui::Text("integration steps");
                t_simulator.numIntegrationSteps.draw(50.0f);

                ImGui::Text("integration step attempts");
                t_simulator.integrationStepsAttempted.draw(50.0f);

                ImGui::Text("integration steps / integration step attempts");
                t_simulator.stepsOverAttempts.draw(50.0f);
            }
        }

        void draw_coords_tab() {
            ImGui::InputText("search", t_coords.filter, sizeof(t_coords.filter));
            ImGui::SameLine();
            if (std::strlen(t_coords.filter) > 0 and ImGui::Button("clear")) {
                t_coords.filter[0] = '\0';
            }

            ImGui::Dummy(ImVec2{0.0f, 5.0f});

            ImGui::Checkbox("sort", &t_coords.sort_by_name);
            ImGui::SameLine();
            ImGui::Checkbox("rotational", &t_coords.show_rotational);
            ImGui::SameLine();
            ImGui::Checkbox("translational", &t_coords.show_translational);

            ImGui::Checkbox("coupled", &t_coords.show_coupled);

            // get coords

            scratch.coords.clear();
            get_coordinates(model, latest_state, scratch.coords);

            // apply filters etc.

            int coordtypes_to_filter_out = 0;
            if (not t_coords.show_rotational) {
                coordtypes_to_filter_out |= Rotational;
            }
            if (not t_coords.show_translational) {
                coordtypes_to_filter_out |= Translational;
            }
            if (not t_coords.show_coupled) {
                coordtypes_to_filter_out |= Coupled;
            }

            auto it = std::remove_if(scratch.coords.begin(), scratch.coords.end(), [&](Coordinate const& c) {
                if (c.type & coordtypes_to_filter_out) {
                    return true;
                }

                if (c.name->find(t_coords.filter) == c.name->npos) {
                    return true;
                }

                return false;
            });
            scratch.coords.erase(it, scratch.coords.end());

            if (t_coords.sort_by_name) {
                std::sort(scratch.coords.begin(), scratch.coords.end(), [](Coordinate const& c1, Coordinate const& c2) {
                    return *c1.name < *c2.name;
                });
            }

            // render sliders

            ImGui::Dummy(ImVec2{0.0f, 10.0f});
            ImGui::Text("Coordinates (%i)", static_cast<int>(scratch.coords.size()));
            ImGui::Separator();

            int i = 0;
            for (Coordinate const& c : scratch.coords) {
                ImGui::PushID(i++);
                draw_coordinate_slider(c);
                ImGui::PopID();
            }
        }

        void draw_coordinate_slider(Coordinate const& coord) {
            // lock button
            if (coord.locked) {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, dark_red);
            }

            if (ImGui::Button(coord.locked ? "u" : "l")) {
                coord.ptr->setLocked(latest_state, not false);
                on_user_edited_state();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(150);

            float v = coord.value;
            if (ImGui::SliderFloat(coord.name->c_str(), &v, coord.min, coord.max)) {
                coord.ptr->setValue(latest_state, static_cast<double>(v));
                on_user_edited_state();
            }

            if (coord.locked) {
                ImGui::PopStyleColor();
            }
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

        void draw_muscles_tab() {
            // extract muscles details from model
            scratch.muscles.clear();
            get_muscle_stats(model, latest_state, scratch.muscles);

            // allow user filtering
            ImGui::InputText("filter muscles", t_muscs.filter, sizeof(t_muscs.filter));
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::InputFloat("min length", &t_muscs.min_len);
            ImGui::InputFloat("max length", &t_muscs.max_len);
            ImGui::Checkbox("inverse range", &t_muscs.inverse_range);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Separator();

            if (ImGui::BeginCombo(
                    "sort by", t_muscs.sorting_choices[t_muscs.current_sort_choice], ImGuiComboFlags_None)) {
                for (size_t n = 0; n < t_muscs.sorting_choices.size(); n++) {
                    bool is_selected = (t_muscs.current_sort_choice == n);
                    if (ImGui::Selectable(t_muscs.sorting_choices[n], is_selected)) {
                        t_muscs.current_sort_choice = n;
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Checkbox("reverse results", &t_muscs.reverse_results);

            // apply muscle sorting
            assert(not t_muscs.sorting_choices.empty());
            assert(t_muscs.current_sort_choice < t_muscs.sorting_choices.size());

            switch (t_muscs.current_sort_choice) {
            case 0: {  // sort muscles by length
                std::sort(
                    scratch.muscles.begin(), scratch.muscles.end(), [](Muscle_stat const& m1, Muscle_stat const& m2) {
                        return m1.length > m2.length;
                    });
                break;
            }
            case 1: {  // sort muscles by tendon strain
                std::sort(
                    scratch.muscles.begin(), scratch.muscles.end(), [&](Muscle_stat const& m1, Muscle_stat const& m2) {
                        return m1.ptr->getTendonStrain(latest_state) > m2.ptr->getTendonStrain(latest_state);
                    });
                break;
            }
            default:
                break;  // skip sorting
            }

            if (t_muscs.reverse_results) {
                std::reverse(scratch.muscles.begin(), scratch.muscles.end());
            }

            // draw muscle list
            for (Muscle_stat const& musc : scratch.muscles) {
                if (musc.name->find(t_muscs.filter) != musc.name->npos) {
                    bool in_range = t_muscs.min_len <= musc.length and musc.length <= t_muscs.max_len;
                    if (t_muscs.inverse_range) {
                        in_range = not in_range;
                    }
                    if (in_range) {
                        ImGui::Text(
                            "%s    l = %.3f, strain =  %.3f %%, force = %.3f",
                            musc.name->c_str(),
                            static_cast<double>(musc.length),
                            100.0 * musc.ptr->getTendonStrain(latest_state),
                            musc.ptr->getTendonForce(latest_state));
                    }
                }
            }
        }

        void draw_moment_arms_tab() {
            ImGui::Text("Moment arms");
            ImGui::Separator();

            ImGui::Columns(2);
            // lhs: muscle selection
            {
                scratch.muscles.clear();
                get_muscle_stats(model, latest_state, scratch.muscles);

                // usability: sort by name
                std::sort(
                    scratch.muscles.begin(), scratch.muscles.end(), [](Muscle_stat const& m1, Muscle_stat const& m2) {
                        return *m1.name < *m2.name;
                    });

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
                ImGui::BeginChild(
                    "MomentArmPlotMuscleSelection",
                    ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260),
                    false,
                    window_flags);
                for (Muscle_stat const& m : scratch.muscles) {
                    if (ImGui::Selectable(m.name->c_str(), m.name == t_momentarms.selected_musc)) {
                        t_momentarms.selected_musc = m.name;
                    }
                }
                ImGui::EndChild();
                ImGui::NextColumn();
            }
            // rhs: coord selection
            {
                scratch.coords.clear();
                get_coordinates(model, latest_state, scratch.coords);

                // usability: sort by name
                std::sort(scratch.coords.begin(), scratch.coords.end(), [](Coordinate const& c1, Coordinate const& c2) {
                    return *c1.name < *c2.name;
                });

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
                ImGui::BeginChild(
                    "MomentArmPlotCoordSelection",
                    ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260),
                    false,
                    window_flags);
                for (Coordinate const& c : scratch.coords) {
                    if (ImGui::Selectable(c.name->c_str(), c.name == t_momentarms.selected_coord)) {
                        t_momentarms.selected_coord = c.name;
                    }
                }
                ImGui::EndChild();
                ImGui::NextColumn();
            }
            ImGui::Columns();

            if (t_momentarms.selected_musc and t_momentarms.selected_coord) {
                if (ImGui::Button("+ add plot")) {
                    auto it =
                        std::find_if(scratch.muscles.begin(), scratch.muscles.end(), [this](Muscle_stat const& ms) {
                            return ms.name == t_momentarms.selected_musc;
                        });
                    assert(it != scratch.muscles.end());

                    auto it2 = std::find_if(scratch.coords.begin(), scratch.coords.end(), [this](Coordinate const& c) {
                        return c.name == t_momentarms.selected_coord;
                    });
                    assert(it2 != scratch.coords.end());

                    auto p = std::make_unique<Moment_arm_plot>();
                    p->muscle_name = *t_momentarms.selected_musc;
                    p->coord_name = *t_momentarms.selected_coord;
                    p->x_begin = it2->min;
                    p->x_end = it2->max;

                    // populate y values
                    compute_moment_arms(*it->ptr, latest_state, *it2->ptr, p->y_vals.data(), p->y_vals.size());
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
                    t_momentarms.selected_coord = nullptr;

                    t_momentarms.plots.push_back(std::move(p));
                }
            }

            if (ImGui::Button("refresh TODO")) {
                // iterate through each plot in plots vector and recompute the moment
                // arms from the UI's current model + state
                throw std::runtime_error{"refreshing moment arm plots NYI"};
            }

            if (not t_momentarms.plots.empty() and ImGui::Button("clear all")) {
                t_momentarms.plots.clear();
            }

            ImGui::Separator();

            ImGui::Columns(2);
            for (size_t i = 0; i < t_momentarms.plots.size(); ++i) {
                Moment_arm_plot const& p = *t_momentarms.plots[i];
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2.0f);
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
                if (ImGui::Button("delete")) {
                    auto it = t_momentarms.plots.begin() + static_cast<int>(i);
                    t_momentarms.plots.erase(it, it + 1);
                }
                ImGui::NextColumn();
            }
            ImGui::Columns();
        }

        void draw_outputs_tab() {
            t_outputs.available.clear();

            // get available outputs from model
            {
                // model-level outputs (e.g. kinetic energy)
                for (auto const& p : model->getOutputs()) {
                    t_outputs.available.push_back(p.second.get());
                }

                // muscle outputs
                for (auto const& musc : model->getComponentList<OpenSim::Muscle>()) {
                    for (auto const& p : musc.getOutputs()) {
                        t_outputs.available.push_back(p.second.get());
                    }
                }
            }

            // apply user filters
            {
                auto it = std::remove_if(
                    t_outputs.available.begin(), t_outputs.available.end(), [&](OpenSim::AbstractOutput const* ao) {
                        append_name(ao, scratch.text, sizeof(scratch.text));
                        return std::strstr(scratch.text, t_outputs.filter) == nullptr;
                    });
                t_outputs.available.erase(it, t_outputs.available.end());
            }

            // input: filter selectable outputs
            ImGui::InputText("filter", t_outputs.filter, sizeof(t_outputs.filter));
            ImGui::Text("%zu available outputs", t_outputs.available.size());

            // list of selectable outputs
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
            if (ImGui::BeginChild("AvailableOutputsSelection", ImVec2(0.0f, 150.0f), true, window_flags)) {
                for (auto const& ao : t_outputs.available) {
                    snprintf(
                        scratch.text,
                        sizeof(scratch.text),
                        "%s/%s",
                        ao->getOwner().getName().c_str(),
                        ao->getName().c_str());
                    if (ImGui::Selectable(scratch.text, ao == t_outputs.selected)) {
                        t_outputs.selected = ao;
                    }
                }
            }
            ImGui::EndChild();

            // buttons: "watch" and "plot"
            if (t_outputs.selected) {
                OpenSim::AbstractOutput const* selected = t_outputs.selected.value();

                // all outputs can be "watch"ed
                if (ImGui::Button("watch selected")) {
                    t_outputs.watches.push_back(selected);
                    t_outputs.selected = std::nullopt;
                }

                // only some outputs can be plotted
                if (produces_doubles(selected)) {
                    ImGui::SameLine();
                    if (ImGui::Button("plot selected")) {
                        t_outputs.plots.emplace_back(selected);
                        t_outputs.selected = std::nullopt;
                    }
                }
            }

            // draw watches
            if (not t_outputs.watches.empty()) {
                ImGui::Text("watches:");
                ImGui::Separator();
                for (OpenSim::AbstractOutput const* ao : t_outputs.watches) {
                    std::string v = ao->getValueAsString(latest_state);
                    ImGui::Text("    %s/%s: %s", ao->getOwner().getName().c_str(), ao->getName().c_str(), v.c_str());
                }
            }

            // draw plots
            if (not t_outputs.plots.empty()) {
                ImGui::Text("plots:");
                ImGui::Separator();

                ImGui::Columns(2);
                for (Output_plot const& p : t_outputs.plots) {
                    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2.0f);
                    p.plot.draw();
                    ImGui::NextColumn();
                    ImGui::Text("%s/%s", p.getOwnerName().c_str(), p.getName().c_str());
                    ImGui::Text("t = %f ms", static_cast<double>(p.plot.latest_x));
                    ImGui::Text("min: %.3f", static_cast<double>(p.plot.min));
                    ImGui::Text("max: %.3f", static_cast<double>(p.plot.max));
                    ImGui::NextColumn();
                }
                ImGui::Columns(1);
            }
        }
    };
}

// screen PIMPL forwarding

osmv::Show_model_screen::Show_model_screen(std::filesystem::path path, osmv::Model model) :
    impl{new Show_model_screen_impl{std::move(path), std::move(model)}} {
}
osmv::Show_model_screen::~Show_model_screen() noexcept = default;

osmv::Event_response osmv::Show_model_screen::on_event(SDL_Event const& e) {
    return impl->handle_event(application(), e);
}

void osmv::Show_model_screen::tick() {
    return impl->tick();
}

void osmv::Show_model_screen::draw() {
    impl->draw(application());
}
