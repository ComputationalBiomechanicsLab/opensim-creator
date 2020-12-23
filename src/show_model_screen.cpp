#include "show_model_screen.hpp"

#include "osmv_config.hpp"

#include "application.hpp"
#include "screen.hpp"
#include "meshes.hpp"
#include "opensim_wrapper.hpp"
#include "loading_screen.hpp"
#include "fd_simulation.hpp"

// OpenGL
#include "gl.hpp"
#include "gl_extensions.hpp"

// glm
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

// imgui
#include "imgui.h"

// c++
#include <string>
#include <vector>
#include <unordered_map>

static constexpr float pi_f = static_cast<float>(M_PI);
static const ImVec4 red{1.0f, 0.0f, 0.0f, 1.0f};
static const ImVec4 dark_red{0.6f, 0.0f, 0.0f, 1.0f};
static const ImVec4 dark_green{0.0f, 0.6f, 0.0f, 1.0f};

template<typename V>
static V& asserting_find(std::vector<std::optional<V>>& meshes, osim::Mesh_id id) {
    assert(id + 1 <= meshes.size());
    assert(meshes[id]);
    return meshes[id].value();
}

struct Shaded_plain_vert final {
    glm::vec3 pos;
    glm::vec3 norm;
};
static_assert(sizeof(Shaded_plain_vert) == 6*sizeof(float));

// renders uniformly colored geometry with gouraud light shading
struct Uniform_color_gouraud_shader final {
    gl::Program program = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(OSMV_SHADERS_DIR "main.vert"),
        gl::CompileFragmentShaderFile(OSMV_SHADERS_DIR "main.frag"));

    static constexpr gl::Attribute location = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute in_normal = gl::AttributeAtLocation(1);

    gl::Uniform_mat4 projMat = gl::GetUniformLocation(program, "projMat");
    gl::Uniform_mat4 viewMat = gl::GetUniformLocation(program, "viewMat");
    gl::Uniform_mat4 modelMat = gl::GetUniformLocation(program, "modelMat");
    gl::Uniform_mat4 normalMat = gl::GetUniformLocation(program, "normalMat");
    gl::Uniform_vec4 rgba = gl::GetUniformLocation(program, "rgba");
    gl::Uniform_vec3 light_pos = gl::GetUniformLocation(program, "lightPos");
    gl::Uniform_vec3 light_color = gl::GetUniformLocation(program, "lightColor");
    gl::Uniform_vec3 view_pos = gl::GetUniformLocation(program, "viewPos");
};

template<typename T>
static gl::Vertex_array create_vao(Uniform_color_gouraud_shader& shader, gl::Sized_array_buffer<T>& vbo) {
    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(shader.location, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
    gl::EnableVertexAttribArray(shader.location);
    gl::VertexAttribPointer(shader.in_normal, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, norm)));
    gl::EnableVertexAttribArray(shader.in_normal);
    gl::BindVertexArray();

    return vao;
}

// renders textured geometry with no lighting shading
struct Plain_texture_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(OSMV_SHADERS_DIR "floor.vert"),
        gl::CompileFragmentShaderFile(OSMV_SHADERS_DIR "floor.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(1);

    gl::Uniform_mat4 projMat = gl::GetUniformLocation(p, "projMat");
    gl::Uniform_mat4 viewMat = gl::GetUniformLocation(p, "viewMat");
    gl::Uniform_mat4 modelMat = gl::GetUniformLocation(p, "modelMat");
    gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");
};

template<typename T>
static gl::Vertex_array create_vao(
        Plain_texture_shader& shader,
        gl::Sized_array_buffer<T>& vbo) {

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(shader.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
    gl::EnableVertexAttribArray(shader.aPos);
    gl::VertexAttribPointer(shader.aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, uv)));
    gl::EnableVertexAttribArray(shader.aTexCoord);
    gl::BindVertexArray();

    return vao;
}

struct Shaded_textured_vert final {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
};
static_assert(sizeof(Shaded_textured_vert) == 8*sizeof(float));

// standard textured quad
// - dimensions [-1, +1] in xy and [0, 0] in z
// - uv coords are (0, 0) bottom-left, (1, 1) top-right
// - normal is +1 in Z, meaning that it faces toward the camera
static constexpr std::array<Shaded_textured_vert, 6> shaded_textured_quad_verts = {{
    {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{-1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}}  // top-left
}};

struct Floor_renderer final {
    Plain_texture_shader s;
    gl::Sized_array_buffer<Shaded_textured_vert> vbo = []() {
        auto copy = shaded_textured_quad_verts;
        for (Shaded_textured_vert& st : copy) {
            st.uv *= 50.0f;  // make chequers smaller
        }
        return gl::Sized_array_buffer<Shaded_textured_vert>{copy.data(), copy.data() + copy.size()};
    }();
    gl::Vertex_array vao = create_vao(s, vbo);
    gl::Texture_2d tex = osmv::generate_chequered_floor_texture();
    glm::mat4 model_mtx = glm::scale(glm::rotate(glm::identity<glm::mat4>(), pi_f/2, {1.0, 0.0, 0.0}), {100.0f, 100.0f, 0.0f});

    void draw(glm::mat4 const& proj, glm::mat4 const& view) {
        gl::UseProgram(s.p);
        gl::Uniform(s.projMat, proj);
        gl::Uniform(s.viewMat, view);
        gl::Uniform(s.modelMat, model_mtx);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(tex);
        gl::Uniform(s.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(vao);
        gl::DrawArrays(GL_TRIANGLES, 0, vbo.sizei());
        gl::BindVertexArray();
    }
};

// renders normals using a geometry shader
struct Normals_shader final {
    gl::Program program = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(OSMV_SHADERS_DIR "normals.vert"),
        gl::CompileFragmentShaderFile(OSMV_SHADERS_DIR "normals.frag"),
        gl::CompileGeometryShaderFile(OSMV_SHADERS_DIR "normals.geom"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);

    gl::Uniform_mat4 projMat = gl::GetUniformLocation(program, "projMat");
    gl::Uniform_mat4 viewMat = gl::GetUniformLocation(program, "viewMat");
    gl::Uniform_mat4 modelMat = gl::GetUniformLocation(program, "modelMat");
    gl::Uniform_mat4 normalMat = gl::GetUniformLocation(program, "normalMat");
};

template<typename T>
static gl::Vertex_array create_vao(
        Normals_shader& shader,
        gl::Sized_array_buffer<T>& vbo) {

    gl::Vertex_array vao = gl::GenVertexArrays();
    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(shader.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
    gl::EnableVertexAttribArray(shader.aPos);
    gl::VertexAttribPointer(shader.aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, norm)));
    gl::EnableVertexAttribArray(shader.aNormal);
    gl::BindVertexArray();
    return vao;
}


// OpenSim mesh shown in main window
struct Scene_mesh final {
    gl::Sized_array_buffer<Shaded_plain_vert> vbo;
    gl::Vertex_array main_vao;
    gl::Vertex_array normal_vao;
};

// upload OpenSim mesh to the GPU
Scene_mesh upload_to_gpu(
        Uniform_color_gouraud_shader& ucgs,
        Normals_shader& sns,
        osim::Untextured_mesh const& m) {
    std::vector<Shaded_plain_vert> v;
    v.reserve(3 * m.triangles.size());
    for (osim::Untextured_triangle const& t : m.triangles) {
        v.push_back(Shaded_plain_vert{t.p1.pos, t.p1.normal});
        v.push_back(Shaded_plain_vert{t.p2.pos, t.p2.normal});
        v.push_back(Shaded_plain_vert{t.p3.pos, t.p3.normal});
    }
    gl::Sized_array_buffer<Shaded_plain_vert> vbo(v.data(), v.data() + v.size());
    gl::Vertex_array main = create_vao(ucgs, vbo);
    gl::Vertex_array norms = create_vao(sns, vbo);

    return Scene_mesh{std::move(vbo), std::move(main), std::move(norms)};
}

namespace osmv {
    struct Show_model_screen_impl final {
        // scratch: shared space that has no content guarantees
        //
        // enables memory recycling, which is important in a low-latency UI
        struct {
            char text[1024 + 1];
            std::vector<osim::Coordinate> coords;
            std::vector<osim::Muscle_stat> muscles;
            osim::Untextured_mesh mesh;
        } scratch;

        // model
        std::string path;
        osim::OSMV_Model model;
        osim::OSMV_State latest_state;

        // 3D rendering
        Uniform_color_gouraud_shader color_shader;
        Normals_shader normals_shader;
        Floor_renderer floor_renderer;
        std::vector<std::optional<Scene_mesh>> meshes;  // indexed by meshid
        osim::State_geometry geom;
        osim::Geometry_loader geom_loader;
        Scene_mesh sphere = [&]() {
            scratch.mesh.triangles.clear();
            osmv::unit_sphere_triangles(scratch.mesh.triangles);
            return upload_to_gpu(color_shader, normals_shader, scratch.mesh);
        }();
        Scene_mesh cylinder = [&]() {
            scratch.mesh.triangles.clear();
            osmv::simbody_cylinder_triangles(12, scratch.mesh.triangles);
            return upload_to_gpu(color_shader, normals_shader, scratch.mesh);
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
        float fd_final_time = 0.5f;
        std::optional<Background_fd_simulation> simulator;

        // tab: coords
        struct {
            char filter[64]{};
            bool sort_by_name = true;
            bool show_rotational = true;
            bool show_translational = true;
            bool show_coupled = true;
        } t_coords;

        struct MA_plot final {
            std::string muscle_name;
            std::string coord_name;
            float x_begin;
            float x_end;
            std::array<float, 50> y_vals;
            float min;
            float max;
        };

        // tab: moment arms
        struct {
            std::string const* selected_musc = nullptr;
            std::string const* selected_coord = nullptr;
            std::vector<std::unique_ptr<MA_plot>> plots;
        } t_mas;

        // tab: muscles
        struct {
            char filter[64]{};
        } t_muscs;

        // tab: outputs
        struct Hacky_output_sparkline final {
            // TODO: this is a hacky implementation that just samples an output
            // whenever the state is updated and stops when the buffer is full
            //
            // a robust solution would install the sampler into the simulator
            // so that sampling isn't FPS-dependent, and so that the appropriate
            // event triggers can be installed into SimTK for regular sampling.

            osim::Available_output ao;
            std::vector<float> y_vals;
            float last_x = 0.0f;
            float min_x_step = 0.005f;  // effectively: 5 ms
            float min = std::numeric_limits<float>::max();
            float max = std::numeric_limits<float>::min();

            Hacky_output_sparkline(osim::Available_output _ao) :
                ao{std::move(_ao)} {
            }
        };

        struct {
            char filter[64]{};
            std::vector<osim::Available_output> available;
            std::optional<osim::Available_output> selected;
            std::vector<osim::Available_output> watches;
            std::vector<std::unique_ptr<Hacky_output_sparkline>> plots;
        } t_outputs;


        Show_model_screen_impl(std::string _path, osim::OSMV_Model model);
        Screen_response handle_event(Application&, SDL_Event&);
        Screen_response tick(Application&);

        void on_user_edited_model();
        void on_user_edited_state();

        void clear_anything_dependent_on_model();
        void clear_anything_dependent_on_state();

        void update_outputs_from_latest_state();

        bool simulator_running();

        void draw(Application&);
        void draw_3d_scene(Application&);
        void draw_imgui_ui(Application&);
        void draw_menu_bar();

        void draw_ui_panel(Application&);
        void draw_ui_tab(Application&);

        void draw_lhs_panel();
        void draw_simulate_tab();
        void draw_coords_tab();
        void draw_coordinate_slider(osim::Coordinate const&);
        void draw_utils_tab();
        void draw_muscles_tab();
        void draw_mas_tab();
        void draw_outputs_tab();
    };
}



// screen PIMPL forwarding

osmv::Show_model_screen::Show_model_screen(std::string path, osim::OSMV_Model model) :
    impl{new Show_model_screen_impl{std::move(path), std::move(model)}} {
}
osmv::Show_model_screen::~Show_model_screen() noexcept = default;
osmv::Screen_response osmv::Show_model_screen::handle_event(Application& s, SDL_Event& e) {
    return impl->handle_event(s, e);
}
osmv::Screen_response osmv::Show_model_screen::tick(Application& a) {
    return impl->tick(a);
}
void osmv::Show_model_screen::draw(osmv::Application& ui) {
    impl->draw(ui);
}



// screen implementation

osmv::Show_model_screen_impl::Show_model_screen_impl(std::string _path, osim::OSMV_Model _model) :
    path{std::move(_path)},
    model{std::move(_model)},
    latest_state{[this]() {
        osim::finalize_from_properties(model);
        osim::OSMV_State s = osim::init_system(model);
        osim::realize_report(model, s);
        return s;
    }()}
{
}


void osmv::Show_model_screen_impl::on_user_edited_model() {
    // these might be invalidated by changing the model because they might
    // contain (e.g.) pointers into the model
    simulator.reset();
    t_mas.selected_musc = nullptr;
    t_mas.selected_coord = nullptr;
    t_outputs.selected = std::nullopt;
    t_outputs.plots.clear();

    latest_state = osim::init_system(model);
    osim::realize_report(model, latest_state);
}

void osmv::Show_model_screen_impl::on_user_edited_state() {
    // stop the simulator whenever a user-initiated state change happens
    if (simulator) {
        simulator->request_stop();
    }
    osim::realize_report(model, latest_state);
}

void osmv::Show_model_screen_impl::update_outputs_from_latest_state() {
    // update currently-recording outputs after the state has been updated

    float sim_millis = 1000.0f * static_cast<float>(osim::simulation_time(latest_state));

    for (std::unique_ptr<Hacky_output_sparkline>& p : t_outputs.plots) {
        Hacky_output_sparkline& hos = *p;

        if (sim_millis < hos.last_x + hos.min_x_step) {
            // latest state is too close to the last datapoint that was plotted
            // so just skip this datapoint
            continue;
        }

        // only certain types of output are plottable at the moment
        assert(hos.ao.is_single_double_val);

        // output val casted to `float` because that's what ImGui uses and we
        // don't need an insane amount of accuracy
        double v = osim::get_single_double_output_val(*hos.ao.handle, latest_state);
        float fv = static_cast<float>(v);

        hos.y_vals.push_back(fv);
        hos.last_x = sim_millis;
        hos.min = std::min(hos.min, fv);
        hos.max = std::max(hos.max, fv);
    }
}

osmv::Screen_response osmv::Show_model_screen_impl::handle_event(Application& ui, SDL_Event& e) {
    ImGuiIO& io = ImGui::GetIO();
    sdl::Window_dimensions window_dims = ui.window_size();
    float aspect_ratio = static_cast<float>(window_dims.w) / static_cast<float>(window_dims.h);

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                return Resp_Please_quit{};
            case SDLK_w:
                wireframe_mode = not wireframe_mode;
                break;
            case SDLK_r: {
                auto km = SDL_GetModState();
                if (km & (KMOD_LCTRL | KMOD_RCTRL)) {
                    return Resp_Transition_to{std::make_unique<osmv::Loading_screen>(path)};
                } else {
                    latest_state = osim::init_system(model);
                    osim::realize_report(model, latest_state);
                }
                break;
            }
            case SDLK_SPACE: {
                if (simulator and simulator->is_running()) {
                    simulator->request_stop();
                } else {
                    simulator.emplace(model, latest_state, fd_final_time);
                }
                break;
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
            return Resp_Ok{};
        }

        if (abs(e.motion.xrel) > 200 or abs(e.motion.yrel) > 200) {
            // probably a frameskip or the mouse was forcibly teleported
            // because it hit the edge of the screen
            return Resp_Ok{};
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
            float x_amt = dx * aspect_ratio * (2.0f * tan(fov / 2.0f) * radius);
            float y_amt = dy * (1.0f / aspect_ratio) * (2.0f * tan(fov / 2.0f) * radius);

            // this assumes the scene is not rotated, so we need to rotate these
            // axes to match the scene's rotation
            glm::vec4 default_panning_axis = { x_amt, y_amt, 0.0f, 1.0f };
            auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), theta, glm::vec3{ 0.0f, 1.0f, 0.0f });
            auto theta_vec = glm::normalize(glm::vec3{ sin(theta), 0.0f, cos(theta) });
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
                ui.move_mouse_to(edge_width, e.motion.y);
            }
            if (e.motion.x - edge_width < 0) {
                ui.move_mouse_to(window_dims.w - edge_width, e.motion.y);
            }
            if (e.motion.y + edge_width > window_dims.h) {
                ui.move_mouse_to(e.motion.x, edge_width);
            }
            if (e.motion.y - edge_width < 0) {
                ui.move_mouse_to(e.motion.x, window_dims.h - edge_width);
            }
        }
    } else if (e.type == SDL_WINDOWEVENT) {
        window_dims = ui.window_size();
        glViewport(0, 0, window_dims.w, window_dims.h);
    } else if (e.type == SDL_MOUSEWHEEL) {
        if (io.WantCaptureMouse) {
            // if ImGUI wants to capture the mouse, then the mouse
            // is probably interacting with an ImGUI panel and,
            // therefore, the dragging/panning shouldn't be handled
            return Resp_Ok{};
        }

        if (e.wheel.y > 0 and radius >= 0.1f) {
            radius *= wheel_sensitivity;
        }

        if (e.wheel.y <= 0 and radius < 100.0f) {
            radius /= wheel_sensitivity;
        }
    }

    return Resp_Ok{};
}

osmv::Screen_response osmv::Show_model_screen_impl::tick(Application &) {
    if (simulator and simulator->try_pop_latest_state(latest_state)) {
        osim::realize_report(model, latest_state);
        update_outputs_from_latest_state();
    }

    return Resp_Ok{};
}

bool osmv::Show_model_screen_impl::simulator_running() {
    return simulator and simulator->is_running();
}

void osmv::Show_model_screen_impl::draw(osmv::Application& ui) {
    // OpenSim: load all geometry in the current state
    geom.clear();
    geom_loader.all_geometry_in(model, latest_state, geom);

    // OpenSim: load any meshes used by the geometry (if necessary)
    for (osim::Mesh_instance const& mi : geom.mesh_instances) {
        if (meshes.size() >= (mi.mesh + 1) and meshes[mi.mesh].has_value()) {
            // it's already loaded
            continue;
        }

        geom_loader.load_mesh(mi.mesh, scratch.mesh);
        meshes.resize(std::max(meshes.size(), mi.mesh + 1));
        meshes[mi.mesh] = upload_to_gpu(color_shader, normals_shader, scratch.mesh);
    }


    // draw everything

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);

    if (gamma_correction) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    draw_3d_scene(ui);
    draw_imgui_ui(ui);
}

// draw main 3D scene
void osmv::Show_model_screen_impl::draw_3d_scene(Application& ui) {
    // camera: at a fixed position pointing at a fixed origin. The "camera"
    // works by translating + rotating all objects around that origin. Rotation
    // is expressed as polar coordinates. Camera panning is represented as a
    // translation vector.

    glm::mat4 view_mtx = [&]() {
        auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), -theta, glm::vec3{ 0.0f, 1.0f, 0.0f });
        auto theta_vec = glm::normalize(glm::vec3{ sin(theta), 0.0f, cos(theta) });
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
            radius * sin(theta) * cos(phi),
            radius * sin(phi),
            radius * cos(theta) * cos(phi)
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

            Scene_mesh& md = asserting_find(meshes, m.mesh);
            gl::BindVertexArray(md.main_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, md.vbo.sizei());
            gl::BindVertexArray();
        }

        // debugging: draw unit cylinder
        if (show_unit_cylinder) {
            gl::BindVertexArray(cylinder.main_vao);

            gl::Uniform(color_shader.rgba, glm::vec4{0.9f, 0.9f, 0.9f, 1.0f});
            gl::Uniform(color_shader.modelMat, glm::identity<glm::mat4>());
            gl::Uniform(color_shader.normalMat, glm::identity<glm::mat4>());

            gl::DrawArrays(GL_TRIANGLES, 0, cylinder.vbo.sizei());

            gl::BindVertexArray();
        }

        // debugging: draw light location
        if (show_light) {
            gl::BindVertexArray(sphere.main_vao);

            gl::Uniform(color_shader.rgba, glm::vec4{1.0f, 1.0f, 0.0f, 0.3f});
            auto xform = glm::scale(glm::translate(glm::identity<glm::mat4>(), light_pos), {0.05, 0.05, 0.05});
            gl::Uniform(color_shader.modelMat, xform);
            gl::Uniform(color_shader.normalMat, glm::transpose(glm::inverse(xform)));
            gl::DrawArrays(GL_TRIANGLES, 0, sphere.vbo.sizei());

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

            Scene_mesh& md = asserting_find(meshes, m.mesh);
            gl::BindVertexArray(md.normal_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, md.vbo.sizei());
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
            if (ImGui::BeginTabItem(path.c_str())) {
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
            simulator.emplace(model, latest_state, fd_final_time);
        }
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    if (ImGui::Button("reset [r]")) {
        latest_state = osim::init_system(model);
        osim::realize_report(model, latest_state);
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
        bool throttling = ui.is_fps_throttling();
        if (ImGui::Checkbox("fps_throttle", &throttling)) {
            if (throttling) {
                ui.enable_fps_throttling();
            } else {
                ui.disable_fps_throttling();
            }
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
    osim::get_coordinates(model, latest_state, scratch.coords);


    // apply filters etc.

    int coordtypes_to_filter_out = 0;
    if (not t_coords.show_rotational) {
        coordtypes_to_filter_out |= osim::Rotational;
    }
    if (not t_coords.show_translational) {
        coordtypes_to_filter_out |= osim::Translational;
    }
    if (not t_coords.show_coupled) {
        coordtypes_to_filter_out |= osim::Coupled;
    }

    auto it = std::remove_if(scratch.coords.begin(), scratch.coords.end(), [&](osim::Coordinate const& c) {
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
        std::sort(scratch.coords.begin(), scratch.coords.end(), [](osim::Coordinate const& c1, osim::Coordinate const& c2) {
            return *c1.name < *c2.name;
        });
    }


    // render sliders

    ImGui::Dummy(ImVec2{0.0f, 10.0f});
    ImGui::Text("Coordinates (%i)", static_cast<int>(scratch.coords.size()));
    ImGui::Separator();

    int i = 0;
    for (osim::Coordinate const& c : scratch.coords) {
        ImGui::PushID(i++);
        draw_coordinate_slider(c);
        ImGui::PopID();
    }
}

void osmv::Show_model_screen_impl::draw_coordinate_slider(osim::Coordinate const& c) {
    // lock button
    if (c.locked) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, dark_red);
    }

    char const* btn_label = c.locked ? "u" : "l";
    if (ImGui::Button(btn_label)) {
        if (c.locked) {
            osim::unlock_coord(*c.ptr, latest_state);
        } else {
            osim::lock_coord(*c.ptr, latest_state);
        }
        on_user_edited_state();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);

    float v = c.value;
    if (ImGui::SliderFloat(c.name->c_str(), &v, c.min, c.max)) {
        osim::set_coord_value(*c.ptr, latest_state, static_cast<double>(v));
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
        osim::disable_wrapping_surfaces(model);
        on_user_edited_model();
    }
    ImGui::SameLine();
    if (ImGui::Button("enable")) {
        osim::enable_wrapping_surfaces(model);
        on_user_edited_model();
    }
}

void osmv::Show_model_screen_impl::draw_muscles_tab() {
    // extract muscles details from model
    scratch.muscles.clear();
    osim::get_muscle_stats(model, latest_state, scratch.muscles);

    // sort muscles alphabetically by name
    std::sort(scratch.muscles.begin(), scratch.muscles.end(), [](osim::Muscle_stat const& m1, osim::Muscle_stat const& m2) {
        return *m1.name < *m2.name;
    });

    // allow user filtering
    ImGui::InputText("filter muscles", t_muscs.filter, sizeof(t_muscs.filter));
    ImGui::Separator();

    // draw muscle list
    for (osim::Muscle_stat const& musc : scratch.muscles) {
        if (musc.name->find(t_muscs.filter) != musc.name->npos) {
            ImGui::Text("%s (len = %.3f)", musc.name->c_str(), musc.length);
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
        osim::get_muscle_stats(model, latest_state, scratch.muscles);

        // usability: sort by name
        std::sort(scratch.muscles.begin(), scratch.muscles.end(), [](osim::Muscle_stat const& m1, osim::Muscle_stat const& m2) {
            return *m1.name < *m2.name;
        });

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("MomentArmPlotMuscleSelection", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260), false, window_flags);
        for (osim::Muscle_stat const& m : scratch.muscles) {
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
        osim::get_coordinates(model, latest_state, scratch.coords);

        // usability: sort by name
        std::sort(scratch.coords.begin(), scratch.coords.end(), [](osim::Coordinate const& c1, osim::Coordinate const& c2) {
            return *c1.name < *c2.name;
        });

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("MomentArmPlotCoordSelection", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260), false, window_flags);
        for (osim::Coordinate const& c : scratch.coords) {
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
                                   [this](osim::Muscle_stat const& ms) {
                    return ms.name == t_mas.selected_musc;
            });
            assert(it != scratch.muscles.end());

            auto it2 = std::find_if(scratch.coords.begin(),
                                    scratch.coords.end(),
                                    [this](osim::Coordinate const& c) {
                    return c.name == t_mas.selected_coord;
            });
            assert(it2 != scratch.coords.end());

            auto p = std::make_unique<MA_plot>();
            p->muscle_name = *t_mas.selected_musc;
            p->coord_name = *t_mas.selected_coord;
            p->x_begin = it2->min;
            p->x_end = it2->max;

            // populate y values
            osim::compute_moment_arms(
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
        MA_plot const& p = *t_mas.plots[i];
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
            auto it = t_mas.plots.begin() + i;
            t_mas.plots.erase(it, it + 1);
        }
        ImGui::NextColumn();
    }
    ImGui::Columns();
}

void osmv::Show_model_screen_impl::draw_outputs_tab() {
    t_outputs.available.clear();
    osim::get_available_outputs(model, t_outputs.available);

    // apply filters
    {
        auto it = std::remove_if(
                    t_outputs.available.begin(),
                    t_outputs.available.end(),
                    [&](osim::Available_output const& ao) {
                snprintf(scratch.text, sizeof(scratch.text), "%s/%s", ao.owner_name->c_str(), ao.output_name->c_str());
                return std::strstr(scratch.text, t_outputs.filter) == nullptr;
        });
        t_outputs.available.erase(it, t_outputs.available.end());
    }


    ImGui::InputText("filter", t_outputs.filter, sizeof(t_outputs.filter));
    ImGui::Text("%zu available outputs", t_outputs.available.size());

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

    if (t_outputs.selected) {
        osim::Available_output const& selected = t_outputs.selected.value();

        // can watch any output because there's a string method in OpenSim
        if (ImGui::Button("watch selected")) {
            t_outputs.watches.push_back(selected);
            t_outputs.selected = std::nullopt;
        }

        // can only plot single-value double outputs
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
        for (osim::Available_output const& ao : t_outputs.watches) {
            std::string v = osim::get_output_val(*ao.handle, latest_state);
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
                hos.y_vals.data(),
                static_cast<int>(hos.y_vals.size()),
                0,
                nullptr,
                std::numeric_limits<float>::min(),
                std::numeric_limits<float>::max(),
                ImVec2(0, 100.0f));
            ImGui::NextColumn();
            ImGui::Text("%s/%s", hos.ao.owner_name->c_str(), hos.ao.output_name->c_str());
            ImGui::Text("n = %zu", hos.y_vals.size());
            ImGui::Text("t = %f", static_cast<double>(hos.last_x));
            ImGui::Text("min: %.3f", static_cast<double>(hos.min));
            ImGui::Text("max: %.2f", static_cast<double>(hos.max));
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }
}
