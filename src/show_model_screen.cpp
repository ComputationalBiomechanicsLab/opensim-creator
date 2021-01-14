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
#include <OpenSim/Simulation/Model/Model.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

// info for a (data) output declared by the model
//
// pointers in this struct are dependent on the model: only use this in short-lived contexts
// and don't let it survive during a model edit or model destruction
struct Available_output final {
    OpenSim::AbstractOutput const* handle;
};

static void snprintf(Available_output const& ao, char* buf, size_t n) {
    std::snprintf(buf, n, "%s/%s", ao.handle->getOwner().getName().c_str(), ao.handle->getName().c_str());
}

static bool produces_doubles(Available_output const& ao) noexcept {
    return (not ao.handle->isListOutput()) and dynamic_cast<OpenSim::Output<double> const*>(ao.handle) != nullptr;
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

namespace osmv {

    // holds a fixed number of Y datapoints that are assumed to be evenly spaced in X
    //
    // if the number of datapoints "pushed" onto the sparkline exceeds the (fixed) capacity then
    // the datapoints will be halved (reducing resolution) to make room for more
    struct Hacky_output_sparkline final {
        static constexpr float min_x_step = 0.005f;
        static constexpr size_t max_datapoints = 256;
        static_assert(max_datapoints % 2 == 0, "num datapoints must be even because the impl uses integer division");

        Available_output ao;
        std::array<float, max_datapoints> data;
        size_t n = 0;
        float x_step = min_x_step;
        float latest_x = -min_x_step;
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();

        Hacky_output_sparkline(Available_output _ao) : ao{std::move(_ao)} {
        }

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

            if (n == max_datapoints) {
                // too many datapoints recorded: half the resolution of the
                // sparkline to accomodate more datapoints being added
                size_t halfway = n / 2;
                for (size_t i = 0; i < halfway; ++i) {
                    data[i] = data[2 * i];
                }
                n = halfway;
                x_step *= 2.0f;
            }

            data[n++] = y;
            latest_x = x;
            min = std::min(min, y);
            max = std::max(max, y);
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

        // gui (camera, lighting, etc.)
        Renderer renderer;

        // simulator
        float fd_final_time = 0.4f;
        std::optional<Fd_simulation> simulator;

        // tab: coords
        struct {
            char filter[64]{};
            bool sort_by_name = true;
            bool show_rotational = true;
            bool show_translational = true;
            bool show_coupled = true;
        } t_coords;

        // tab: moment arms
        struct {
            std::string const* selected_musc = nullptr;
            std::string const* selected_coord = nullptr;
            std::vector<std::unique_ptr<Moment_arm_plot>> plots;
        } t_mas;

        // tab: muscles
        struct {
            char filter[64]{};
        } t_muscs;

        // tab: outputs
        struct {
            char filter[64]{};
            std::vector<Available_output> available;
            std::optional<Available_output> selected;
            std::vector<Available_output> watches;
            std::vector<std::unique_ptr<Hacky_output_sparkline>> plots;
        } t_outputs;

        Show_model_screen_impl(std::filesystem::path, osmv::Model);
        void handle_event(Application&, SDL_Event&);
        void tick();
        void draw(Application&);

        void on_user_edited_model();
        void on_user_edited_state();

        void update_outputs_from_latest_state();

        bool simulator_running();

        void draw_3d_scene(Application&);

        void draw_menu_bar();
        void draw_ui_panel(Application&);
        void draw_ui_tab(Application&);

        void draw_lhs_panel();
        void draw_simulate_tab();
        void draw_coords_tab();
        void draw_coordinate_slider(Coordinate const&);
        void draw_utils_tab();
        void draw_muscles_tab();
        void draw_mas_tab();
        void draw_outputs_tab();
    };
}

// screen PIMPL forwarding

osmv::Show_model_screen::Show_model_screen(std::filesystem::path path, osmv::Model model) :
    impl{new Show_model_screen_impl{std::move(path), std::move(model)}} {
}
osmv::Show_model_screen::~Show_model_screen() noexcept = default;

void osmv::Show_model_screen::on_event(SDL_Event& e) {
    return impl->handle_event(application(), e);
}

void osmv::Show_model_screen::tick() {
    return impl->tick();
}

void osmv::Show_model_screen::draw() {
    impl->draw(application());
}

// screen implementation

osmv::Show_model_screen_impl::Show_model_screen_impl(std::filesystem::path _path, osmv::Model _model) :

    path{std::move(_path)},
    model{std::move(_model)},
    latest_state{[this]() {
        model->finalizeFromProperties();
        osmv::State s{model->initSystem()};
        model->realizeReport(s);
        return s;
    }()} {
}

void osmv::Show_model_screen_impl::on_user_edited_model() {
    // these might be invalidated by changing the model because they might
    // contain (e.g.) pointers into the model
    simulator = std::nullopt;
    t_mas.selected_musc = nullptr;
    t_mas.selected_coord = nullptr;
    t_outputs.selected = std::nullopt;
    t_outputs.plots.clear();

    latest_state = model->initSystem();
    model->realizeReport(latest_state);
}

void osmv::Show_model_screen_impl::on_user_edited_state() {
    // stop the simulator whenever a user-initiated state change happens
    if (simulator) {
        simulator->request_stop();
    }

    model->realizeReport(latest_state);

    // clear all output plots, because the user *probably* wants to see fresh
    // data after resetting the state
    for (std::unique_ptr<Hacky_output_sparkline> const& p : t_outputs.plots) {
        p->clear();
    }
}

// update currently-recording outputs after the state has been updated
void osmv::Show_model_screen_impl::update_outputs_from_latest_state() {
    float sim_millis = 1000.0f * static_cast<float>(latest_state->getTime());

    for (std::unique_ptr<Hacky_output_sparkline>& p : t_outputs.plots) {
        Hacky_output_sparkline& hos = *p;

        // only certain types of output are plottable at the moment
        auto* o = dynamic_cast<OpenSim::Output<double> const*>(hos.ao.handle);
        assert(o);
        double v = o->getValue(latest_state);
        float fv = static_cast<float>(v);

        hos.push_datapoint(sim_millis, fv);
    }
}

// handle top-level UI event (user click, user drag, etc.)
void osmv::Show_model_screen_impl::handle_event(Application& app, SDL_Event& e) {

    ImGuiIO& io = ImGui::GetIO();
    sdl::Window_dimensions window_dims = app.window_size();

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_r: {
            auto km = SDL_GetModState();
            if (km & (KMOD_LCTRL | KMOD_RCTRL)) {
                app.request_transition<Loading_screen>(path);
                return;
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
                simulator.emplace(Fd_simulation_params{Model{model}, State{latest_state}, fd_final_time});
            }
            break;
        case SDLK_ESCAPE:
            app.request_transition<Splash_screen>();
            return;
        }
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        if (io.WantCaptureMouse) {
            // if ImGUI wants to capture the mouse, then the mouse
            // is probably interacting with an ImGUI panel and,
            // therefore, the dragging/panning shouldn't be handled
            return;
        }
    } else if (e.type == SDL_WINDOWEVENT) {
        window_dims = app.window_size();
        glViewport(0, 0, window_dims.w, window_dims.h);
    } else if (e.type == SDL_MOUSEWHEEL) {
        if (io.WantCaptureMouse) {
            // if ImGUI wants to capture the mouse, then the mouse
            // is probably interacting with an ImGUI panel and,
            // therefore, the dragging/panning shouldn't be handled
            return;
        }
    }

    if (renderer.on_event(app, e)) {
        return;
    }
}

// "tick" the UI state (usually, used for updating animations etc.)
void osmv::Show_model_screen_impl::tick() {
    // grab the latest state (if any) from the simulator and (if updated)
    // update the UI to reflect the latest state
    if (simulator and simulator->try_pop_latest_state(latest_state)) {
        model->realizeReport(latest_state);
        update_outputs_from_latest_state();
    }
}

bool osmv::Show_model_screen_impl::simulator_running() {
    return simulator and simulator->is_running();
}

// draw a frame of the UI
void osmv::Show_model_screen_impl::draw(osmv::Application& ui) {
    renderer.draw(ui, model, latest_state);

    draw_menu_bar();
    draw_ui_panel(ui);
    draw_lhs_panel();
}

void osmv::Show_model_screen_impl::draw_menu_bar() {
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

void osmv::Show_model_screen_impl::draw_ui_panel(Application& ui) {
    bool b = true;
    ImGuiWindowFlags flags = ImGuiWindowFlags_Modal;

    if (ImGui::Begin("UI", &b, flags)) {
        draw_ui_tab(ui);
    }
    ImGui::End();
}

void osmv::Show_model_screen_impl::draw_lhs_panel() {
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
                draw_mas_tab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

static const ImVec4 red{1.0f, 0.0f, 0.0f, 1.0f};
static const ImVec4 dark_green{0.0f, 0.6f, 0.0f, 1.0f};

void osmv::Show_model_screen_impl::draw_simulate_tab() {
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
            Fd_simulation_params params{Model{model}, State{latest_state}, fd_final_time};
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
    ImGui::SliderFloat("final time", &fd_final_time, 0.01f, 20.0f);

    if (simulator) {
        std::chrono::milliseconds wall_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(simulator->wall_duration());
        double wall_secs = static_cast<double>(wall_ms.count()) / 1000.0;
        double sim_secs = simulator->sim_current_time();
        double pct_completed = sim_secs / simulator->sim_final_time() * 100.0;

        ImGui::Dummy(ImVec2{0.0f, 20.0f});
        ImGui::Text("simulator stats");
        ImGui::Separator();
        ImGui::Text("status: %s", simulator->status_description());
        ImGui::Text("progress: %.2f %%", pct_completed);
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        ImGui::Text("simulation time: %.2f", sim_secs);
        ImGui::Text("wall time: %.2f secs", wall_secs);
        ImGui::Text("sim_time/wall_time: %.4f", sim_secs / wall_secs);
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        ImGui::Text("prescribeQ calls: %i", simulator->num_prescribeq_calls());
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        ImGui::Text("States sent to UI thread: %i", simulator->num_states_popped());
        ImGui::Text("Avg. UI overhead: %.5f %%", 100.0 * simulator->avg_ui_overhead());
    }
}

void osmv::Show_model_screen_impl::draw_ui_tab(Application& ui) {
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
    ImGui::Checkbox("show_light", &renderer.show_light);
    ImGui::Checkbox("show_unit_cylinder", &renderer.show_unit_cylinder);
    ImGui::Checkbox("show_floor", &renderer.show_floor);
    ImGui::Checkbox("gamma_correction", &renderer.gamma_correction);
    {
        bool throttling = ui.is_throttling_fps();
        if (ImGui::Checkbox("fps_throttle", &throttling)) {
            ui.is_throttling_fps(throttling);
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

void osmv::Show_model_screen_impl::draw_coords_tab() {
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

static const ImVec4 dark_red{0.6f, 0.0f, 0.0f, 1.0f};

void osmv::Show_model_screen_impl::draw_coordinate_slider(Coordinate const& c) {
    // lock button
    if (c.locked) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, dark_red);
    }

    if (ImGui::Button(c.locked ? "u" : "l")) {
        c.ptr->setLocked(latest_state, not false);
        on_user_edited_state();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);

    float v = c.value;
    if (ImGui::SliderFloat(c.name->c_str(), &v, c.min, c.max)) {
        c.ptr->setValue(latest_state, static_cast<double>(v));
        on_user_edited_state();
    }

    if (c.locked) {
        ImGui::PopStyleColor();
    }
}

void osmv::Show_model_screen_impl::draw_utils_tab() {
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

    if (ImGui::Button("redify")) {
        OpenSim::Model& m = model;
        for (OpenSim::Geometry& g : m.updComponentList<OpenSim::Geometry>()) {
            g.setColor({1.0, 0.0, 0.0});
        }
    }

    thread_local float color = 0.0f;
    if (ImGui::SliderFloat("red", &color, 0.0f, 1.0f)) {
        OpenSim::Model& m = model;
        for (OpenSim::Geometry& g : m.updComponentList<OpenSim::Geometry>()) {
            g.setColor({static_cast<double>(color), 0.0, 0.0});
        }
    }
}

void osmv::Show_model_screen_impl::draw_muscles_tab() {
    // extract muscles details from model
    scratch.muscles.clear();
    get_muscle_stats(model, latest_state, scratch.muscles);

    // sort muscles alphabetically by name
    std::sort(scratch.muscles.begin(), scratch.muscles.end(), [](Muscle_stat const& m1, Muscle_stat const& m2) {
        return *m1.name < *m2.name;
    });

    // allow user filtering
    ImGui::InputText("filter muscles", t_muscs.filter, sizeof(t_muscs.filter));
    ImGui::Separator();

    // draw muscle list
    for (Muscle_stat const& musc : scratch.muscles) {
        if (musc.length > 0.5) {
            if (musc.name->find(t_muscs.filter) != musc.name->npos) {
                ImGui::Text("%s (len = %.3f)", musc.name->c_str(), static_cast<double>(musc.length));
            }
        }
    }
}

void osmv::Show_model_screen_impl::draw_mas_tab() {
    ImGui::Text("Moment arms");
    ImGui::Separator();

    ImGui::Columns(2);
    // lhs: muscle selection
    {
        scratch.muscles.clear();
        get_muscle_stats(model, latest_state, scratch.muscles);

        // usability: sort by name
        std::sort(scratch.muscles.begin(), scratch.muscles.end(), [](Muscle_stat const& m1, Muscle_stat const& m2) {
            return *m1.name < *m2.name;
        });

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild(
            "MomentArmPlotMuscleSelection",
            ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260),
            false,
            window_flags);
        for (Muscle_stat const& m : scratch.muscles) {
            if (ImGui::Selectable(m.name->c_str(), m.name == t_mas.selected_musc)) {
                t_mas.selected_musc = m.name;
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
            if (ImGui::Selectable(c.name->c_str(), c.name == t_mas.selected_coord)) {
                t_mas.selected_coord = c.name;
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();
    }
    ImGui::Columns();

    if (t_mas.selected_musc and t_mas.selected_coord) {
        if (ImGui::Button("+ add plot")) {
            auto it = std::find_if(scratch.muscles.begin(), scratch.muscles.end(), [this](Muscle_stat const& ms) {
                return ms.name == t_mas.selected_musc;
            });
            assert(it != scratch.muscles.end());

            auto it2 = std::find_if(scratch.coords.begin(), scratch.coords.end(), [this](Coordinate const& c) {
                return c.name == t_mas.selected_coord;
            });
            assert(it2 != scratch.coords.end());

            auto p = std::make_unique<Moment_arm_plot>();
            p->muscle_name = *t_mas.selected_musc;
            p->coord_name = *t_mas.selected_coord;
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
            t_mas.selected_coord = nullptr;

            t_mas.plots.push_back(std::move(p));
        }
    }

    if (ImGui::Button("refresh TODO")) {
        // iterate through each plot in plots vector and recompute the moment
        // arms from the UI's current model + state
        throw std::runtime_error{"refreshing moment arm plots NYI"};
    }

    if (not t_mas.plots.empty() and ImGui::Button("clear all")) {
        t_mas.plots.clear();
    }

    ImGui::Separator();

    ImGui::Columns(2);
    for (size_t i = 0; i < t_mas.plots.size(); ++i) {
        Moment_arm_plot const& p = *t_mas.plots[i];
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
            auto it = t_mas.plots.begin() + static_cast<int>(i);
            t_mas.plots.erase(it, it + 1);
        }
        ImGui::NextColumn();
    }
    ImGui::Columns();
}

inline bool operator==(Available_output const& a, Available_output const& b) {
    return a.handle == b.handle;
}

static void get_available_outputs(OpenSim::Model const& m, std::vector<Available_output>& out) {
    // model-level outputs (e.g. kinetic energy)
    for (auto const& p : m.getOutputs()) {
        out.push_back(Available_output{p.second.get()});
    }

    // muscle outputs
    for (auto const& musc : m.getComponentList<OpenSim::Muscle>()) {
        for (auto const& p : musc.getOutputs()) {
            out.push_back(Available_output{p.second.get()});
        }
    }
}

void osmv::Show_model_screen_impl::draw_outputs_tab() {
    t_outputs.available.clear();
    get_available_outputs(model, t_outputs.available);

    // apply user filters
    {
        auto it =
            std::remove_if(t_outputs.available.begin(), t_outputs.available.end(), [&](Available_output const& ao) {
                snprintf(
                    scratch.text,
                    sizeof(scratch.text),
                    "%s/%s",
                    ao.handle->getOwner().getName().c_str(),
                    ao.handle->getName().c_str());
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
                ao.handle->getOwner().getName().c_str(),
                ao.handle->getName().c_str());
            if (ImGui::Selectable(scratch.text, ao == t_outputs.selected)) {
                t_outputs.selected = ao;
            }
        }
    }
    ImGui::EndChild();

    // buttons: "watch" and "plot"
    if (t_outputs.selected) {
        Available_output const& selected = t_outputs.selected.value();

        // all outputs can be "watch"ed
        if (ImGui::Button("watch selected")) {
            t_outputs.watches.push_back(selected);
            t_outputs.selected = std::nullopt;
        }

        // only some outputs can be plotted
        if (produces_doubles(selected)) {
            ImGui::SameLine();
            if (ImGui::Button("plot selected")) {
                auto p = std::make_unique<Hacky_output_sparkline>(selected);
                t_outputs.plots.push_back(std::move(p));
                t_outputs.selected = std::nullopt;
            }
        }
    }

    // draw watches
    if (not t_outputs.watches.empty()) {
        ImGui::Text("watches:");
        ImGui::Separator();
        for (Available_output const& ao : t_outputs.watches) {
            std::string v = ao.handle->getValueAsString(latest_state);
            ImGui::Text(
                "    %s/%s: %s", ao.handle->getOwner().getName().c_str(), ao.handle->getName().c_str(), v.c_str());
        }
    }

    // draw plots
    if (not t_outputs.plots.empty()) {
        ImGui::Text("plots:");
        ImGui::Separator();

        ImGui::Columns(2);
        for (std::unique_ptr<Hacky_output_sparkline> const& p : t_outputs.plots) {
            Hacky_output_sparkline const& hos = *p;

            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2.0f);
            ImGui::PlotLines(
                "",
                hos.data.data(),
                static_cast<int>(hos.n),
                0,
                nullptr,
                std::numeric_limits<float>::min(),
                std::numeric_limits<float>::max(),
                ImVec2(0, 100.0f));
            ImGui::NextColumn();
            ImGui::Text("%s/%s", hos.ao.handle->getOwner().getName().c_str(), hos.ao.handle->getName().c_str());
            ImGui::Text("n = %zu", hos.n);
            ImGui::Text("t = %f", static_cast<double>(hos.latest_x));
            ImGui::Text("min: %.3f", static_cast<double>(hos.min));
            ImGui::Text("max: %.3f", static_cast<double>(hos.max));
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }
}
