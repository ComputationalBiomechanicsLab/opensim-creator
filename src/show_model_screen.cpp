#include "show_model_screen.hpp"

#include "osmv_config.hpp"

#include "application.hpp"
#include "screen.hpp"
#include "3d_common.hpp"
#include "opensim_wrapper.hpp"
#include "loading_screen.hpp"
#include "splash_screen.hpp"
#include "fd_simulation.hpp"
#include "os.hpp"
#include "cfg.hpp"
#include "gl.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"

#include <string>
#include <vector>
#include <stdexcept>

namespace osmv {
    // renders uniformly colored geometry with Gouraud shading
    struct Uniform_color_gouraud_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(cfg::shader_path("main.vert")),
            gl::Compile<gl::Fragment_shader>(cfg::shader_path("main.frag")));

        static constexpr gl::Attribute location{0};
        static constexpr gl::Attribute in_normal{1};

        gl::Uniform_mat4 projMat = {program, "projMat"};
        gl::Uniform_mat4 viewMat = {program, "viewMat"};
        gl::Uniform_mat4 modelMat = {program, "modelMat"};
        gl::Uniform_mat4 normalMat = {program, "normalMat"};
        gl::Uniform_vec4 rgba = {program, "rgba"};
        gl::Uniform_vec3 light_pos = {program, "lightPos"};
        gl::Uniform_vec3 light_color = {program, "lightColor"};
        gl::Uniform_vec3 view_pos = {program, "viewPos"};

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(location);
            gl::VertexAttribPointer(in_normal, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, normal)));
            gl::EnableVertexAttribArray(in_normal);
            gl::BindVertexArray();

            return vao;
        }
    };

    // renders textured geometry with no shading
    struct Plain_texture_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(cfg::shader_path("floor.vert")),
            gl::Compile<gl::Fragment_shader>(cfg::shader_path("floor.frag")));

        static constexpr gl::Attribute aPos{0};
        static constexpr gl::Attribute aTexCoord{1};

        gl::Uniform_mat4 projMat = {p, "projMat"};
        gl::Uniform_mat4 viewMat = {p, "viewMat"};
        gl::Uniform_mat4 modelMat = {p, "modelMat"};
        gl::Uniform_sampler2d uSampler0 = {p, "uSampler0"};

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, texcoord)));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            return vao;
        }
    };

    // renders normals using a geometry shader
    struct Normals_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(cfg::shader_path("normals.vert")),
            gl::Compile<gl::Fragment_shader>(cfg::shader_path("normals.frag")),
            gl::Compile<gl::Geometry_shader>(cfg::shader_path("normals.geom")));

        static constexpr gl::Attribute aPos{0};
        static constexpr gl::Attribute aNormal{1};

        gl::Uniform_mat4 projMat = {program, "projMat"};
        gl::Uniform_mat4 viewMat = {program, "viewMat"};
        gl::Uniform_mat4 modelMat = {program, "modelMat"};
        gl::Uniform_mat4 normalMat = {program, "normalMat"};

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, normal)));
            gl::EnableVertexAttribArray(aNormal);
            gl::BindVertexArray();
            return vao;
        }
    };

    // uses the floor shader to render the scene's chequered floor
    struct Floor_renderer final {
        Plain_texture_shader s;

        gl::Array_bufferT<Shaded_textured_vert> vbo = []() {
            auto copy = shaded_textured_quad_verts;
            for (Shaded_textured_vert& st : copy) {
                st.texcoord *= 50.0f;  // make chequers smaller
            }
            return gl::Array_bufferT<Shaded_textured_vert>{copy};
        }();

        gl::Vertex_array vao = Plain_texture_shader::create_vao(vbo);
        gl::Texture_2d floor_texture = osmv::generate_chequered_floor_texture();
        glm::mat4 model_mtx = glm::scale(glm::rotate(glm::identity<glm::mat4>(), pi_f/2, {1.0, 0.0, 0.0}), {100.0f, 100.0f, 0.0f});

        void draw(glm::mat4 const& proj, glm::mat4 const& view) {
            gl::UseProgram(s.p);

            gl::Uniform(s.projMat, proj);
            gl::Uniform(s.viewMat, view);
            gl::Uniform(s.modelMat, model_mtx);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(floor_texture);
            gl::Uniform(s.uSampler0, gl::texture_index<GL_TEXTURE0>());

            gl::BindVertexArray(vao);
            gl::DrawArrays(GL_TRIANGLES, 0, vbo.sizei());
            gl::BindVertexArray();
        }
    };

    // OpenSim mesh shown in main window
    class Mesh_on_gpu final {
        gl::Array_bufferT<osmv::Untextured_vert> vbo;
        gl::Vertex_array main_vao;
        gl::Vertex_array normal_vao;

    public:
        Mesh_on_gpu(std::vector<Untextured_vert> const& m) :
            vbo{m},
            main_vao{Uniform_color_gouraud_shader::create_vao(vbo)},
            normal_vao{Normals_shader::create_vao(vbo)} {
        }

        template<typename T>
        gl::Vertex_array& vao_for() noexcept;

        int sizei() const noexcept {
            return vbo.sizei();
        }
    };

    template<>
    gl::Vertex_array& Mesh_on_gpu::vao_for<Uniform_color_gouraud_shader>() noexcept {
        return main_vao;
    }

    template<>
    gl::Vertex_array& Mesh_on_gpu::vao_for<Normals_shader>() noexcept {
        return normal_vao;
    }

    // holds a fixed number of Y datapoints that are assumed to be evenly spaced in X
    //
    // if the number of datapoints "pushed" onto the sparkline exceeds the (fixed) capacity then
    // the datapoints will be halved (reducing resolution) to make room for more
    struct Hacky_output_sparkline final {
        static constexpr float min_x_step = 0.005f;
        static constexpr size_t max_datapoints = 256;
        static_assert(max_datapoints % 2 == 0, "num datapoints must be even because the impl uses integer division");

        osmv::Available_output ao;
        std::array<float, max_datapoints> data;
        size_t n = 0;
        float x_step = min_x_step;
        float latest_x = -min_x_step;
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();

        Hacky_output_sparkline(osmv::Available_output _ao) :
            ao{std::move(_ao)} {
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
                size_t halfway = n/2;
                for (size_t i = 0; i < halfway; ++i) {
                    data[i] = data[2*i];
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
            std::vector<Untextured_vert> mesh;
        } scratch;

        // model
        std::filesystem::path path;
        osmv::Model model;
        osmv::State latest_state;

        // 3D rendering
        Uniform_color_gouraud_shader color_shader;
        Normals_shader normals_shader;
        Floor_renderer floor_renderer;
        std::vector<std::optional<Mesh_on_gpu>> meshes;  // indexed by meshid
        osmv::State_geometry geom;
        osmv::Geometry_loader geom_loader;
        Mesh_on_gpu sphere = [&]() {
            scratch.mesh.clear();
            osmv::unit_sphere_triangles(scratch.mesh);
            return Mesh_on_gpu{scratch.mesh};
        }();
        Mesh_on_gpu cylinder = [&]() {
            scratch.mesh.clear();
            osmv::simbody_cylinder_triangles(12, scratch.mesh);
            return Mesh_on_gpu{scratch.mesh};
        }();

        // gui (camera, lighting, etc.)
        float radius = 5.0f;
        float theta = 0.88f;
        float phi =  0.4f;
        glm::vec3 pan = {0.3f, -0.5f, 0.0f};
        float fov = 120.0f;
        bool dragging = false;
        bool panning = false;
        float sensitivity = 1.0f;
        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        glm::vec3 light_color = {0.9607f, 0.9176f, 0.8863f};
        bool wireframe_mode = false;
        bool show_light = false;
        bool show_unit_cylinder = false;
        bool gamma_correction = false;
        bool show_mesh_normals = false;
        bool show_floor = true;
        float wheel_sensitivity = 0.9f;

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
            std::vector<osmv::Available_output> available;
            std::optional<osmv::Available_output> selected;
            std::vector<osmv::Available_output> watches;
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
        void draw_imgui_ui(Application&);
        void draw_menu_bar();

        void draw_ui_panel(Application&);
        void draw_ui_tab(Application&);

        void draw_lhs_panel();
        void draw_simulate_tab();
        void draw_coords_tab();
        void draw_coordinate_slider(osmv::Coordinate const&);
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

osmv::Show_model_screen_impl::Show_model_screen_impl(
        std::filesystem::path _path,
        osmv::Model _model) :

    path{std::move(_path)},
    model{std::move(_model)},
    latest_state{[this]() {
        osmv::finalize_from_properties(model);
        osmv::State s{osmv::init_system(model)};
        osmv::realize_report(model, s);
        return s;
    }()}
{
}

void osmv::Show_model_screen_impl::on_user_edited_model() {
    // these might be invalidated by changing the model because they might
    // contain (e.g.) pointers into the model
    simulator = std::nullopt;
    t_mas.selected_musc = nullptr;
    t_mas.selected_coord = nullptr;
    t_outputs.selected = std::nullopt;
    t_outputs.plots.clear();

    latest_state = osmv::init_system(model);
    osmv::realize_report(model, latest_state);
}

void osmv::Show_model_screen_impl::on_user_edited_state() {
    // stop the simulator whenever a user-initiated state change happens
    if (simulator) {
        simulator->request_stop();
    }

    osmv::realize_report(model, latest_state);

    // clear all output plots, because the user *probably* wants to see fresh
    // data after resetting the state
    for (std::unique_ptr<Hacky_output_sparkline> const& p : t_outputs.plots) {
        p->clear();
    }
}

// update currently-recording outputs after the state has been updated
void osmv::Show_model_screen_impl::update_outputs_from_latest_state() {
    float sim_millis = 1000.0f * static_cast<float>(osmv::simulation_time(latest_state));

    for (std::unique_ptr<Hacky_output_sparkline>& p : t_outputs.plots) {
        Hacky_output_sparkline& hos = *p;

        // only certain types of output are plottable at the moment
        assert(hos.ao.is_single_double_val);

        double v = osmv::get_output_val_double(*hos.ao.handle, latest_state);
        float fv = static_cast<float>(v);

        hos.push_datapoint(sim_millis, fv);
    }
}

// handle top-level UI event (user click, user drag, etc.)
void osmv::Show_model_screen_impl::handle_event(Application& app, SDL_Event& e) {

    ImGuiIO& io = ImGui::GetIO();
    sdl::Window_dimensions window_dims = app.window_size();
    float aspect_ratio = app.aspect_ratio();

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_w:
                wireframe_mode = not wireframe_mode;
                break;
            case SDLK_r: {
                auto km = SDL_GetModState();
                if (km & (KMOD_LCTRL | KMOD_RCTRL)) {
                    auto loading_scr = std::make_unique<osmv::Loading_screen>(path);
                    app.request_transition(std::move(loading_scr));
                    return;
                } else {
                    latest_state = osmv::init_system(model);
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
                auto splash_screen = std::make_unique<osmv::Splash_screen>();
                app.request_transition(std::move(splash_screen));
                return;
            }
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            dragging = true;
            break;
        case SDL_BUTTON_RIGHT:
            panning = true;
            break;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            dragging = false;
            break;
        case SDL_BUTTON_RIGHT:
            panning = false;
            break;
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        if (io.WantCaptureMouse) {
            // if ImGUI wants to capture the mouse, then the mouse
            // is probably interacting with an ImGUI panel and,
            // therefore, the dragging/panning shouldn't be handled
            return;
        }

        if (abs(e.motion.xrel) > 200 or abs(e.motion.yrel) > 200) {
            // probably a frameskip or the mouse was forcibly teleported
            // because it hit the edge of the screen
            return;
        }

        if (dragging) {
            // alter camera position while dragging
            float dx = -static_cast<float>(e.motion.xrel) / static_cast<float>(window_dims.w);
            float dy = static_cast<float>(e.motion.yrel) / static_cast<float>(window_dims.h);
            theta += 2.0f * static_cast<float>(M_PI) * sensitivity * dx;
            phi += 2.0f * static_cast<float>(M_PI) * sensitivity * dy;
        }

        if (panning) {
            float dx = static_cast<float>(e.motion.xrel) / static_cast<float>(window_dims.w);
            float dy = -static_cast<float>(e.motion.yrel) / static_cast<float>(window_dims.h);

            // how much panning is done depends on how far the camera is from the
            // origin (easy, with polar coordinates) *and* the FoV of the camera.
            float x_amt = dx * aspect_ratio * (2.0f * tanf(fov / 2.0f) * radius);
            float y_amt = dy * (1.0f / aspect_ratio) * (2.0f * tanf(fov / 2.0f) * radius);

            // this assumes the scene is not rotated, so we need to rotate these
            // axes to match the scene's rotation
            glm::vec4 default_panning_axis = { x_amt, y_amt, 0.0f, 1.0f };
            auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), theta, glm::vec3{ 0.0f, 1.0f, 0.0f });
            auto theta_vec = glm::normalize(glm::vec3{ sinf(theta), 0.0f, cosf(theta) });
            auto phi_axis = glm::cross(theta_vec, glm::vec3{ 0.0, 1.0f, 0.0f });
            auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), phi, phi_axis);

            glm::vec4 panning_axes = rot_phi * rot_theta * default_panning_axis;
            pan.x += panning_axes.x;
            pan.y += panning_axes.y;
            pan.z += panning_axes.z;
        }

        // wrap mouse if it hits edges
        if (dragging or panning) {
            constexpr int edge_width = 5;
            if (e.motion.x + edge_width > window_dims.w) {
                app.move_mouse_to(edge_width, e.motion.y);
            }
            if (e.motion.x - edge_width < 0) {
                app.move_mouse_to(window_dims.w - edge_width, e.motion.y);
            }
            if (e.motion.y + edge_width > window_dims.h) {
                app.move_mouse_to(e.motion.x, edge_width);
            }
            if (e.motion.y - edge_width < 0) {
                app.move_mouse_to(e.motion.x, window_dims.h - edge_width);
            }
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

        if (e.wheel.y > 0 and radius >= 0.1f) {
            radius *= wheel_sensitivity;
        }

        if (e.wheel.y <= 0 and radius < 100.0f) {
            radius /= wheel_sensitivity;
        }
    }
}

// "tick" the UI state (usually, used for updating animations etc.)
void osmv::Show_model_screen_impl::tick() {
    // grab the latest state (if any) from the simulator and (if updated)
    // update the UI to reflect the latest state
    if (simulator and simulator->try_pop_latest_state(latest_state)) {
        osmv::realize_report(model, latest_state);
        update_outputs_from_latest_state();
    }
}

bool osmv::Show_model_screen_impl::simulator_running() {
    return simulator and simulator->is_running();
}

// draw a frame of the UI
void osmv::Show_model_screen_impl::draw(osmv::Application& ui) {

    // OpenSim: load all geometry in the current state
    //
    // this is a fairly slow thing to put into the draw loop, but means that
    // we can be lazier about monitoring where/when the rendered state changes
    geom.clear();
    geom_loader.all_geometry_in(model, latest_state, geom);
    for (osmv::Mesh_instance const& mi : geom.mesh_instances) {
        if (meshes.size() >= (mi.mesh_id + 1) and meshes[mi.mesh_id].has_value()) {
            // the mesh is already loaded
            continue;
        }

        geom_loader.load_mesh(mi.mesh_id, scratch.mesh);
        meshes.resize(std::max(meshes.size(), mi.mesh_id + 1));
        meshes[mi.mesh_id] = Mesh_on_gpu{scratch.mesh};
    }

    // HACK: the scene might contain blended (alpha < 1.0f) elements. These must
    // be drawn last, ideally back-to-front (not yet implemented); otherwise,
    // OpenGL will early-discard after the vertex shader when they fail a
    // depth test
    std::sort(geom.mesh_instances.begin(), geom.mesh_instances.end(), [](Mesh_instance const& a, Mesh_instance const& b) {
        return a.rgba.a > b.rgba.a;
    });

    // draw
    gl::ClearColor(0.99f, 0.98f, 0.96f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);

    draw_3d_scene(ui);
    draw_imgui_ui(ui);
}

template<typename V>
static V& asserting_find(std::vector<std::optional<V>>& meshes, osmv::Mesh_id id) {
    assert(id + 1 <= meshes.size());
    assert(meshes[id]);
    return meshes[id].value();
}

void osmv::Show_model_screen_impl::draw_3d_scene(Application& ui) {
    // camera: at a fixed position pointing at a fixed origin. The "camera"
    // works by translating + rotating all objects around that origin. Rotation
    // is expressed as polar coordinates. Camera panning is represented as a
    // translation vector.

    glm::mat4 view_mtx = [&]() {
        auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), -theta, glm::vec3{ 0.0f, 1.0f, 0.0f });
        auto theta_vec = glm::normalize(glm::vec3{ sinf(theta), 0.0f, cosf(theta) });
        auto phi_axis = glm::cross(theta_vec, glm::vec3{ 0.0, 1.0f, 0.0f });
        auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), -phi, phi_axis);
        auto pan_translate = glm::translate(glm::identity<glm::mat4>(), pan);
        return glm::lookAt(
            glm::vec3(0.0f, 0.0f, radius),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3{0.0f, 1.0f, 0.0f}) * rot_theta * rot_phi * pan_translate;
    }();

    glm::mat4 proj_mtx = [&]() {
        return glm::perspective(fov, ui.aspect_ratio(), 0.1f, 100.0f);
    }();

    glm::vec3 view_pos = [&]() {
        // polar/spherical to cartesian
        return glm::vec3{
            radius * sinf(theta) * cosf(phi),
            radius * sinf(phi),
            radius * cosf(theta) * cosf(phi)
        };
    }();

    // render elements that have a solid color
    {
        gl::UseProgram(color_shader.program);

        gl::Uniform(color_shader.projMat, proj_mtx);
        gl::Uniform(color_shader.viewMat, view_mtx);
        gl::Uniform(color_shader.light_pos, light_pos);
        gl::Uniform(color_shader.light_color, light_color);
        gl::Uniform(color_shader.view_pos, view_pos);

        // draw model meshes
        for (auto& m : geom.mesh_instances) {
            gl::Uniform(color_shader.rgba, m.rgba);
            gl::Uniform(color_shader.modelMat, m.transform);
            gl::Uniform(color_shader.normalMat, m.normal_xform);

            Mesh_on_gpu& md = asserting_find(meshes, m.mesh_id);
            gl::BindVertexArray(md.vao_for<Uniform_color_gouraud_shader>());
            gl::DrawArrays(GL_TRIANGLES, 0, md.sizei());
            gl::BindVertexArray();
        }

        // debugging: draw unit cylinder
        if (show_unit_cylinder) {
            gl::Uniform(color_shader.rgba, glm::vec4{0.9f, 0.9f, 0.9f, 1.0f});
            gl::Uniform(color_shader.modelMat, glm::identity<glm::mat4>());
            gl::Uniform(color_shader.normalMat, glm::identity<glm::mat4>());

            gl::BindVertexArray(cylinder.vao_for<Uniform_color_gouraud_shader>());
            gl::DrawArrays(GL_TRIANGLES, 0, cylinder.sizei());
            gl::BindVertexArray();
        }

        // debugging: draw light location
        if (show_light) {
            gl::Uniform(color_shader.rgba, glm::vec4{1.0f, 1.0f, 0.0f, 0.3f});
            auto xform = glm::scale(glm::translate(glm::identity<glm::mat4>(), light_pos), {0.05, 0.05, 0.05});
            gl::Uniform(color_shader.modelMat, xform);
            gl::Uniform(color_shader.normalMat, glm::transpose(glm::inverse(xform)));

            gl::BindVertexArray(sphere.vao_for<Uniform_color_gouraud_shader>());
            gl::DrawArrays(GL_TRIANGLES, 0, sphere.sizei());
            gl::BindVertexArray();
        }
    }

    // floor is rendered with a texturing program
    if (show_floor) {
        floor_renderer.draw(proj_mtx, view_mtx);
    }

    // debugging: draw mesh normals
    if (show_mesh_normals) {
        gl::UseProgram(normals_shader.program);
        gl::Uniform(normals_shader.projMat, proj_mtx);
        gl::Uniform(normals_shader.viewMat, view_mtx);

        for (auto& m : geom.mesh_instances) {
            gl::Uniform(normals_shader.modelMat, m.transform);
            gl::Uniform(normals_shader.normalMat, m.normal_xform);

            Mesh_on_gpu& md = asserting_find(meshes, m.mesh_id);
            gl::BindVertexArray(md.vao_for<Normals_shader>());
            gl::DrawArrays(GL_TRIANGLES, 0, md.sizei());
            gl::BindVertexArray();
        }
    }
}

void osmv::Show_model_screen_impl::draw_imgui_ui(Application& ui) {
    draw_menu_bar();
    draw_ui_panel(ui);
    draw_lhs_panel();
}

void osmv::Show_model_screen_impl::draw_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginTabBar("MainTabBar")) {
            if (ImGui::BeginTabItem(path.filename().c_str())) {
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndMainMenuBar();
    }
}

void osmv::Show_model_screen_impl::draw_ui_panel(Application & ui) {
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
            Fd_simulation_params params{
                Model{model},
                State{latest_state},
                fd_final_time
            };
            simulator.emplace(std::move(params));
        }
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    if (ImGui::Button("reset [r]")) {
        latest_state = osmv::init_system(model);
        on_user_edited_state();
    }

    ImGui::Dummy(ImVec2{0.0f, 20.0f});

    ImGui::Text("simulation config");
    ImGui::Separator();
    ImGui::SliderFloat("final time", &fd_final_time, 0.01f, 20.0f);

    if (simulator) {
        std::chrono::milliseconds wall_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(simulator->wall_duration());
        double wall_secs = static_cast<double>(wall_ms.count())/1000.0;
        double sim_secs = simulator->sim_current_time();
        double pct_completed = sim_secs/simulator->sim_final_time() * 100.0;

        ImGui::Dummy(ImVec2{0.0f, 20.0f});
        ImGui::Text("simulator stats");
        ImGui::Separator();
        ImGui::Text("status: %s", simulator->status_description());
        ImGui::Text("progress: %.2f %%", pct_completed);
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        ImGui::Text("simulation time: %.2f", sim_secs);
        ImGui::Text("wall time: %.2f secs", wall_secs);
        ImGui::Text("sim_time/wall_time: %.4f", sim_secs/wall_secs);
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
        theta = pi_f/2.0f;
        phi = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Back")) {
        // assumes models tend to point upwards in Y and forwards in +X
        theta = 3.0f * (pi_f/2.0f);
        phi = 0.0f;
    }

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    if (ImGui::Button("Left")) {
        // assumes models tend to point upwards in Y and forwards in +X
        // (so sidewards is theta == 0 or PI)
        theta = pi_f;
        phi = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Right")) {
        // assumes models tend to point upwards in Y and forwards in +X
        // (so sidewards is theta == 0 or PI)
        theta = 0.0f;
        phi = 0.0f;
    }

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    if (ImGui::Button("Top")) {
        theta = 0.0f;
        phi = pi_f/2.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Bottom")) {
        theta = 0.0f;
        phi = 3.0f * (pi_f/2.0f);
    }

    ImGui::NewLine();

    ImGui::SliderFloat("radius", &radius, 0.0f, 10.0f);
    ImGui::SliderFloat("theta", &theta, 0.0f, 2.0f*pi_f);
    ImGui::SliderFloat("phi", &phi, 0.0f, 2.0f*pi_f);
    ImGui::NewLine();
    ImGui::SliderFloat("pan_x", &pan.x, -100.0f, 100.0f);
    ImGui::SliderFloat("pan_y", &pan.y, -100.0f, 100.0f);
    ImGui::SliderFloat("pan_z", &pan.z, -100.0f, 100.0f);

    ImGui::NewLine();
    ImGui::Text("Lighting:");
    ImGui::SliderFloat("light_x", &light_pos.x, -30.0f, 30.0f);
    ImGui::SliderFloat("light_y", &light_pos.y, -30.0f, 30.0f);
    ImGui::SliderFloat("light_z", &light_pos.z, -30.0f, 30.0f);
    ImGui::ColorEdit3("light_color", reinterpret_cast<float*>(&light_color));
    ImGui::Checkbox("show_light", &show_light);
    ImGui::Checkbox("show_unit_cylinder", &show_unit_cylinder);
    ImGui::Checkbox("show_floor", &show_floor);
    ImGui::Checkbox("gamma_correction", &gamma_correction);
    {
        bool throttling = ui.is_throttling_fps();
        if (ImGui::Checkbox("fps_throttle", &throttling)) {
            ui.is_throttling_fps(throttling);
        }
    }
    ImGui::Checkbox("show_mesh_normals", &show_mesh_normals);

    ImGui::NewLine();
    ImGui::Text("Interaction: ");
    if (dragging) {
        ImGui::SameLine();
        ImGui::Text("rotating ");
    }
    if (panning) {
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
    osmv::get_coordinates(model, latest_state, scratch.coords);


    // apply filters etc.

    int coordtypes_to_filter_out = 0;
    if (not t_coords.show_rotational) {
        coordtypes_to_filter_out |= osmv::Rotational;
    }
    if (not t_coords.show_translational) {
        coordtypes_to_filter_out |= osmv::Translational;
    }
    if (not t_coords.show_coupled) {
        coordtypes_to_filter_out |= osmv::Coupled;
    }

    auto it = std::remove_if(scratch.coords.begin(), scratch.coords.end(), [&](osmv::Coordinate const& c) {
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
        std::sort(scratch.coords.begin(), scratch.coords.end(), [](osmv::Coordinate const& c1, osmv::Coordinate const& c2) {
            return *c1.name < *c2.name;
        });
    }


    // render sliders

    ImGui::Dummy(ImVec2{0.0f, 10.0f});
    ImGui::Text("Coordinates (%i)", static_cast<int>(scratch.coords.size()));
    ImGui::Separator();

    int i = 0;
    for (osmv::Coordinate const& c : scratch.coords) {
        ImGui::PushID(i++);
        draw_coordinate_slider(c);
        ImGui::PopID();
    }
}

static const ImVec4 dark_red{0.6f, 0.0f, 0.0f, 1.0f};

void osmv::Show_model_screen_impl::draw_coordinate_slider(osmv::Coordinate const& c) {
    // lock button
    if (c.locked) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, dark_red);
    }

    char const* btn_label = c.locked ? "u" : "l";
    if (ImGui::Button(btn_label)) {
        if (c.locked) {
            osmv::unlock_coord(*c.ptr, latest_state);
        } else {
            osmv::lock_coord(*c.ptr, latest_state);
        }
        on_user_edited_state();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);

    float v = c.value;
    if (ImGui::SliderFloat(c.name->c_str(), &v, c.min, c.max)) {
        osmv::set_coord_value(*c.ptr, latest_state, static_cast<double>(v));
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
        osmv::disable_wrapping_surfaces(model);
        on_user_edited_model();
    }
    ImGui::SameLine();
    if (ImGui::Button("enable")) {
        osmv::enable_wrapping_surfaces(model);
        on_user_edited_model();
    }
}

void osmv::Show_model_screen_impl::draw_muscles_tab() {
    // extract muscles details from model
    scratch.muscles.clear();
    osmv::get_muscle_stats(model, latest_state, scratch.muscles);

    // sort muscles alphabetically by name
    std::sort(scratch.muscles.begin(), scratch.muscles.end(), [](osmv::Muscle_stat const& m1, osmv::Muscle_stat const& m2) {
        return *m1.name < *m2.name;
    });

    // allow user filtering
    ImGui::InputText("filter muscles", t_muscs.filter, sizeof(t_muscs.filter));
    ImGui::Separator();

    // draw muscle list
    for (osmv::Muscle_stat const& musc : scratch.muscles) {
        if (musc.name->find(t_muscs.filter) != musc.name->npos) {
            ImGui::Text("%s (len = %.3f)", musc.name->c_str(), static_cast<double>(musc.length));
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
        osmv::get_muscle_stats(model, latest_state, scratch.muscles);

        // usability: sort by name
        std::sort(scratch.muscles.begin(), scratch.muscles.end(), [](osmv::Muscle_stat const& m1, osmv::Muscle_stat const& m2) {
            return *m1.name < *m2.name;
        });

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("MomentArmPlotMuscleSelection", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260), false, window_flags);
        for (osmv::Muscle_stat const& m : scratch.muscles) {
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
        osmv::get_coordinates(model, latest_state, scratch.coords);

        // usability: sort by name
        std::sort(scratch.coords.begin(), scratch.coords.end(), [](osmv::Coordinate const& c1, osmv::Coordinate const& c2) {
            return *c1.name < *c2.name;
        });

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("MomentArmPlotCoordSelection", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260), false, window_flags);
        for (osmv::Coordinate const& c : scratch.coords) {
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
            auto it = std::find_if(scratch.muscles.begin(),
                                   scratch.muscles.end(),
                                   [this](osmv::Muscle_stat const& ms) {
                    return ms.name == t_mas.selected_musc;
            });
            assert(it != scratch.muscles.end());

            auto it2 = std::find_if(scratch.coords.begin(),
                                    scratch.coords.end(),
                                    [this](osmv::Coordinate const& c) {
                    return c.name == t_mas.selected_coord;
            });
            assert(it2 != scratch.coords.end());

            auto p = std::make_unique<Moment_arm_plot>();
            p->muscle_name = *t_mas.selected_musc;
            p->coord_name = *t_mas.selected_coord;
            p->x_begin = it2->min;
            p->x_end = it2->max;

            // populate y values
            osmv::compute_moment_arms(
                *it->ptr,
                latest_state,
                *it2->ptr,
                p->y_vals.data(),
                p->y_vals.size());
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
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth()/2.0f);
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

void osmv::Show_model_screen_impl::draw_outputs_tab() {
    t_outputs.available.clear();
    osmv::get_available_outputs(model, t_outputs.available);

    // apply user filters
    {
        auto it = std::remove_if(
                    t_outputs.available.begin(),
                    t_outputs.available.end(),
                    [&](osmv::Available_output const& ao) {
                snprintf(scratch.text, sizeof(scratch.text), "%s/%s", ao.owner_name->c_str(), ao.output_name->c_str());
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
            snprintf(scratch.text, sizeof(scratch.text), "%s/%s", ao.owner_name->c_str(), ao.output_name->c_str());
            if (ImGui::Selectable(scratch.text, ao == t_outputs.selected)) {
                t_outputs.selected = ao;
            }
        }
    }
    ImGui::EndChild();

    // buttons: "watch" and "plot"
    if (t_outputs.selected) {
        osmv::Available_output const& selected = t_outputs.selected.value();

        // all outputs can be "watch"ed
        if (ImGui::Button("watch selected")) {
            t_outputs.watches.push_back(selected);
            t_outputs.selected = std::nullopt;
        }

        // only some outputs can be plotted
        if (selected.is_single_double_val) {
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
        for (osmv::Available_output const& ao : t_outputs.watches) {
            std::string v = osmv::get_output_val_any(*ao.handle, latest_state);
            ImGui::Text("    %s/%s: %s", ao.owner_name->c_str(), ao.output_name->c_str(), v.c_str());
        }
    }

    // draw plots
    if (not t_outputs.plots.empty()) {
        ImGui::Text("plots:");
        ImGui::Separator();

        ImGui::Columns(2);
        for (std::unique_ptr<Hacky_output_sparkline> const& p : t_outputs.plots) {
            Hacky_output_sparkline const& hos = *p;

            ImGui::SetNextItemWidth(ImGui::GetWindowWidth()/2.0f);
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
            ImGui::Text("%s/%s", hos.ao.owner_name->c_str(), hos.ao.output_name->c_str());
            ImGui::Text("n = %zu", hos.n);
            ImGui::Text("t = %f", static_cast<double>(hos.latest_x));
            ImGui::Text("min: %.3f", static_cast<double>(hos.min));
            ImGui::Text("max: %.3f", static_cast<double>(hos.max));
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }
}
