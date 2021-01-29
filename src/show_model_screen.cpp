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
#include "screen.hpp"
#include "simple_model_renderer.hpp"
#include "splash_screen.hpp"

#include <OpenSim/Actuators/Millard2012EquilibriumMuscle.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui.h>

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    static void append_name(OpenSim::AbstractOutput const* ao, char* buf, size_t n) {
        std::snprintf(buf, n, "%s/%s", ao->getOwner().getName().c_str(), ao->getName().c_str());
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

            prescribeQcalls.push_datapoint(sim_time, sim.num_prescribeq_calls());
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
                ImGui::PushStyleColor(ImGuiCol_Button, red);
                if (ImGui::Button("stop [SPC]")) {
                    simulator->request_stop();
                }
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, dark_green);
                if (ImGui::Button("start [SPC]")) {
                    osmv::Fd_simulation_params params{
                        osmv::Model{shown_model}, osmv::State{shown_state}, fd_final_time};
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
            ImGui::SliderFloat(" ", &fd_final_time, 0.01f, 20.0f);
            ImGui::NextColumn();
            ImGui::Columns();

            // ImGui::SliderFloat("final time", &fd_final_time, 0.01f, 20.0f);

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
}

namespace osmv {
    struct Show_model_screen_impl final {
        // scratch: shared space that has no content guarantees
        struct {
            char text[1024 + 1];
            std::vector<Coordinate> coords;
            std::vector<Muscle_stat> muscles;
        } scratch;

        std::filesystem::path model_path;
        osmv::Model model;
        osmv::State latest_state;

        Selected_component selected_component;

        Simple_model_renderer renderer;

        Coordinates_tab_data coords_tab;
        Simulator_tab simulator_tab;
        Momentarms_tab_data mas_tab;
        Muscles_tab_data muscles_tab;
        Outputs_tab_data outputs_tab;

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
        }

        // handle top-level UI event (user click, user drag, etc.)
        Event_response handle_event(Application& app, SDL_Event const& e) {
            ImGuiIO& io = ImGui::GetIO();

            if (e.type == SDL_KEYDOWN) {
                if (io.WantCaptureKeyboard) {
                    // if ImGUI wants to capture the keyboard, then the keyboard
                    // is probably interacting with an ImGUI panel
                    return Event_response::ignored;
                }

                switch (e.key.keysym.sym) {
                case SDLK_r: {

                    // CTRL + R: reload the model from scratch
                    SDL_Keymod km = SDL_GetModState();
                    if (km & (KMOD_LCTRL | KMOD_RCTRL)) {
                        app.request_screen_transition<Loading_screen>(model_path);
                        return Event_response::handled;
                    }

                    // R: reset the model to its initial state
                    latest_state = model->initSystem();
                    on_user_edited_state();

                    return Event_response::handled;
                }
                case SDLK_SPACE: {
                    if (simulator_tab.simulator and simulator_tab.simulator->is_running()) {
                        simulator_tab.simulator->request_stop();
                    } else {
                        simulator_tab.simulator.emplace(
                            Fd_simulation_params{Model{model}, State{latest_state}, simulator_tab.fd_final_time});
                    }
                    break;
                case SDLK_ESCAPE:
                    app.request_screen_transition<Splash_screen>();
                    return Event_response::handled;
                case SDLK_c:
                    // clear selection
                    selected_component = nullptr;
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
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (io.WantCaptureMouse) {
                    // if ImGUI wants to capture the mouse, then the mouse
                    // is probably interacting with an ImGUI panel and,
                    // therefore, the dragging/panning shouldn't be handled
                    return Event_response::ignored;
                }

            } else if (e.type == SDL_MOUSEBUTTONUP) {
                if (io.WantCaptureMouse) {
                    // if ImGUI wants to capture the mouse, then the mouse
                    // is probably interacting with an ImGUI panel and,
                    // therefore, the dragging/panning shouldn't be handled
                    return Event_response::ignored;
                }

                // otherwise, maybe they're trying to select something in the viewport, so
                // check if they are hovered over a component and select it if they are
                if (e.button.button == SDL_BUTTON_RIGHT and renderer.hovered_component) {
                    selected_component = renderer.hovered_component;
                }
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
            return renderer.on_event(app, e) ? Event_response::handled : Event_response::ignored;
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

            // draw OpenSim 3D model using renderer
            renderer.draw(app, model, latest_state, selected_component);

            // overlay: if the user is hovering over a component, write the component's name
            //          next to the mouse
            if (renderer.hovered_component) {
                OpenSim::Component const& c = *renderer.hovered_component;
                sdl::Mouse_state m = sdl::GetMouseState();
                ImVec2 pos{static_cast<float>(m.x + 20), static_cast<float>(m.y)};
                ImGui::GetBackgroundDrawList()->AddText(pos, 0xff0000ff, c.getName().c_str());
            }

            // menu bar
            if (false && ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginTabBar("MainTabBar")) {
                    if (ImGui::BeginTabItem(model_path.filename().string().c_str())) {
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::EndMainMenuBar();
            }

            // draw lhs panel containing tabs (e.g. simulator, UI, muscles)
            draw_main_panel(app);
        }

        void draw_main_panel(Application& app) {
            bool b = true;

            if (ImGui::Begin("Coordinates", &b)) {
                draw_coords_tab();
            }
            ImGui::End();

            if (ImGui::Begin("Simulate", &b)) {
                draw_simulate_tab();
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
            ImGui::ColorEdit3("light_color", reinterpret_cast<float*>(&renderer.light_rgb));
            ImGui::SliderFloat("rim thickness", &renderer.rim_thickness, 0.0f, 0.1f);
            ImGui::Checkbox("draw rims", &renderer.draw_rims);
            ImGui::Checkbox("show_floor", &renderer.show_floor);
            {
                bool throttling = app.is_throttling_fps();
                if (ImGui::Checkbox("fps_throttle", &throttling)) {
                    app.is_throttling_fps(throttling);
                }
            }
            ImGui::Checkbox("show_mesh_normals", &renderer.show_mesh_normals);

            if (ImGui::Button("fullscreen")) {
                app.make_fullscreen();
            }

            if (ImGui::Button("windowed")) {
                app.make_windowed();
            }

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

        void draw_simulate_tab() {
            simulator_tab.draw(renderer, selected_component, model, latest_state);
        }

        void draw_coords_tab() {
            // render filters section
            ImGui::Text("filters:");
            ImGui::Dummy({0.0f, 2.5f});
            ImGui::Separator();

            ImGui::Columns(2);

            ImGui::Text("search");
            ImGui::NextColumn();
            ImGui::PushID("coords search filter");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputText(" ", coords_tab.filter, sizeof(coords_tab.filter));
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::Text("sort alphabetically");
            ImGui::NextColumn();
            ImGui::PushID("coords alphabetical sort");
            ImGui::Checkbox(" ", &coords_tab.sort_by_name);
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::Text("show rotational");
            ImGui::NextColumn();
            ImGui::PushID("rotational coordinates checkbox");
            ImGui::Checkbox(" ", &coords_tab.show_rotational);
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::Text("show translational");
            ImGui::NextColumn();
            ImGui::PushID("translational coordinates checkbox");
            ImGui::Checkbox(" ", &coords_tab.show_rotational);
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::Text("show coupled");
            ImGui::NextColumn();
            ImGui::PushID("coupled coordinates checkbox");
            ImGui::Checkbox(" ", &coords_tab.show_coupled);
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::Columns();

            // load coords
            scratch.coords.clear();
            get_coordinates(model, latest_state, scratch.coords);

            // filter coords
            {
                int coordtypes_to_filter_out = 0;
                if (not coords_tab.show_rotational) {
                    coordtypes_to_filter_out |= Rotational;
                }
                if (not coords_tab.show_translational) {
                    coordtypes_to_filter_out |= Translational;
                }
                if (not coords_tab.show_coupled) {
                    coordtypes_to_filter_out |= Coupled;
                }

                auto should_remove = [&](Coordinate const& c) {
                    if (c.type & coordtypes_to_filter_out) {
                        return true;
                    }

                    if (c.name->find(coords_tab.filter) == c.name->npos) {
                        return true;
                    }

                    return false;
                };

                auto it = std::remove_if(scratch.coords.begin(), scratch.coords.end(), should_remove);
                scratch.coords.erase(it, scratch.coords.end());
            }

            // sort coords
            if (coords_tab.sort_by_name) {
                auto sort_coord = [](Coordinate const& c1, Coordinate const& c2) { return *c1.name < *c2.name; };

                std::sort(scratch.coords.begin(), scratch.coords.end(), sort_coord);
            }

            // render coordinates list
            ImGui::Dummy({0.0f, 10.0f});
            ImGui::Text("coordinates (%zu):", scratch.coords.size());
            ImGui::Dummy({0.0f, 2.5f});
            ImGui::Separator();

            ImGui::Columns(2);
            int i = 0;
            for (Coordinate const& c : scratch.coords) {
                ImGui::PushID(i++);

                ImGui::Text("%s", c.name->c_str());
                ImGui::NextColumn();

                // if locked, color everything red
                if (c.locked) {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, dark_red);
                }

                if (ImGui::Button(c.locked ? "u" : "l")) {
                    c.ptr->setLocked(latest_state, not false);
                    on_user_edited_state();
                }

                ImGui::SameLine();

                float v = c.value;
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::SliderFloat(" ", &v, c.min, c.max)) {
                    c.ptr->setValue(latest_state, static_cast<double>(v));
                    on_user_edited_state();
                }

                if (c.locked) {
                    ImGui::PopStyleColor();
                }
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

        void draw_muscles_tab() {
            // extract muscles details from model
            scratch.muscles.clear();
            get_muscle_stats(model, latest_state, scratch.muscles);

            ImGui::Text("filters:");
            ImGui::Dummy({0.0f, 2.5f});
            ImGui::Separator();

            ImGui::Columns(2);

            ImGui::Text("search");
            ImGui::NextColumn();
            ImGui::PushID("muscles search filter");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputText(" ", muscles_tab.filter, sizeof(muscles_tab.filter));
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::Text("min length");
            ImGui::NextColumn();
            ImGui::PushID("muscles min filter");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputFloat(" ", &muscles_tab.min_len);
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::Text("max length");
            ImGui::NextColumn();
            ImGui::PushID("muscles max filter");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputFloat(" ", &muscles_tab.max_len);
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::Text("inverse length range");
            ImGui::NextColumn();
            ImGui::PushID("muscles inverse range filter");
            ImGui::Checkbox(" ", &muscles_tab.inverse_range);
            ImGui::PopID();
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
            ImGui::PushID("muscles reverse results chechbox");
            ImGui::Checkbox(" ", &muscles_tab.reverse_results);
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::Columns();

            // all user filters handled, transform the muscle list accordingly.

            // filter muscle list
            {
                auto filter_fn = [&](Muscle_stat const& m) {
                    bool in_range = muscles_tab.min_len <= m.length and m.length <= muscles_tab.max_len;

                    if (muscles_tab.inverse_range) {
                        in_range = not in_range;
                    }

                    if (not in_range) {
                        return true;
                    }

                    bool matches_filter = m.name->find(muscles_tab.filter) != m.name->npos;

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
            for (Muscle_stat const& musc : scratch.muscles) {
                ImGui::Text("%s", musc.name->c_str());
                if (ImGui::IsItemHovered()) {
                    renderer.hovered_component = musc.ptr;
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    selected_component = musc.ptr;
                }
                ImGui::NextColumn();
                ImGui::Text("%.3f", static_cast<double>(musc.length));
                ImGui::NextColumn();
                ImGui::Text("%.3f", 100.0 * musc.ptr->getTendonStrain(latest_state));
                ImGui::NextColumn();
                ImGui::Text("%.3f", musc.ptr->getTendonForce(latest_state));
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
                get_muscle_stats(model, latest_state, scratch.muscles);

                // usability: sort by name
                std::sort(
                    scratch.muscles.begin(), scratch.muscles.end(), [](Muscle_stat const& m1, Muscle_stat const& m2) {
                        return *m1.name < *m2.name;
                    });

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
                ImGui::BeginChild(
                    "MomentArmPlotMuscleSelection", ImVec2(ImGui::GetContentRegionAvail().x, 260), false, window_flags);
                for (Muscle_stat const& m : scratch.muscles) {
                    if (ImGui::Selectable(m.name->c_str(), m.name == mas_tab.selected_musc)) {
                        mas_tab.selected_musc = m.name;
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
                get_coordinates(model, latest_state, scratch.coords);

                // usability: sort by name
                std::sort(scratch.coords.begin(), scratch.coords.end(), [](Coordinate const& c1, Coordinate const& c2) {
                    return *c1.name < *c2.name;
                });

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
                ImGui::BeginChild(
                    "MomentArmPlotCoordSelection", ImVec2(ImGui::GetContentRegionAvail().x, 260), false, window_flags);
                for (Coordinate const& c : scratch.coords) {
                    if (ImGui::Selectable(c.name->c_str(), c.name == mas_tab.selected_coord)) {
                        mas_tab.selected_coord = c.name;
                    }
                }
                ImGui::EndChild();
                ImGui::NextColumn();
            }
            ImGui::Columns();

            if (mas_tab.selected_musc and mas_tab.selected_coord) {
                if (ImGui::Button("+ add plot")) {
                    auto it =
                        std::find_if(scratch.muscles.begin(), scratch.muscles.end(), [this](Muscle_stat const& ms) {
                            return ms.name == mas_tab.selected_musc;
                        });
                    assert(it != scratch.muscles.end());

                    auto it2 = std::find_if(scratch.coords.begin(), scratch.coords.end(), [this](Coordinate const& c) {
                        return c.name == mas_tab.selected_coord;
                    });
                    assert(it2 != scratch.coords.end());

                    auto p = std::make_unique<Moment_arm_plot>();
                    p->muscle_name = *mas_tab.selected_musc;
                    p->coord_name = *mas_tab.selected_coord;
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
                ImGui::PushID(i);
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
                ImGui::Text("nothing selected");
                return;
            }

            OpenSim::Component const& c = *selected_component;

            ImGui::Text("name: %s", c.getName().c_str());

            ImGui::Separator();

            size_t i = 0;
            ImGui::Columns(2);
            for (auto const& ptr : c.getOutputs()) {

                OpenSim::AbstractOutput const* ao = ptr.second.get();
                OpenSim::Output<double> const* od = dynamic_cast<OpenSim::Output<double> const*>(ao);
                if (od) {
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
                    selected_component.output_sinks[i++].draw(20.0f);

                    if (ImGui::BeginPopupContextItem(od->getName().c_str())) {
                        if (ImGui::MenuItem("Add to outputs tab")) {
                            outputs_tab.plots.emplace_back(ao);
                        }

                        ImGui::EndPopup();
                    }
                }
                ImGui::NextColumn();

                ImGui::Text("%s", ao->getName().c_str());
                ImGui::NextColumn();
            }
            ImGui::Columns();
        }
    };
}

// screen PIMPL forwarding

osmv::Show_model_screen::Show_model_screen(Application& app, std::filesystem::path path, osmv::Model model) :
    impl{new Show_model_screen_impl{app, std::move(path), std::move(model)}} {
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
