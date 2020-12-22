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

template<typename K, typename V>
static V& asserting_find(std::unordered_map<K, V>& m, K const& k) {
    auto it = m.find(k);
    assert(it != m.end());
    return it->second;
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
struct Show_normals_shader final {
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
        Show_normals_shader& shader,
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
        Show_normals_shader& sns,
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

static Scene_mesh upload_cylinder_to_gpu(
        Uniform_color_gouraud_shader& ucgs,
        Show_normals_shader& sns,
        osim::Untextured_mesh& swap) {
    swap.triangles.clear();
    osmv::simbody_cylinder_triangles(12, swap.triangles);

    return upload_to_gpu(ucgs, sns, swap);
}

static Scene_mesh upload_sphere_to_gpu(
        Uniform_color_gouraud_shader& ucgs,
        Show_normals_shader& sns,
        osim::Untextured_mesh& swap) {
    swap.triangles.clear();
    osmv::unit_sphere_triangles(swap.triangles);

    return upload_to_gpu(ucgs, sns, swap);
}

namespace osmv {
    struct Show_model_screen_impl final {
        std::string path;
        osim::Untextured_mesh mesh_swap_space;
        Uniform_color_gouraud_shader ucgs;
        Show_normals_shader sns;

        Scene_mesh cylinder = upload_cylinder_to_gpu(ucgs, sns, mesh_swap_space);
        Scene_mesh sphere = upload_sphere_to_gpu(ucgs, sns, mesh_swap_space);
        Floor_renderer floor_renderer;

        float radius = 5.0f;
        float wheel_sensitivity = 0.9f;

        float fov = 120.0f;

        bool dragging = false;
        float theta = 0.88f;
        float phi =  0.4f;
        float sensitivity = 1.0f;
        float fd_final_time = 0.5f;

        bool panning = false;
        glm::vec3 pan = {0.3f, -0.5f, 0.0f};

        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        float light_color[3] = {0.9607f, 0.9176f, 0.8863f};
        bool wireframe_mode = false;
        bool show_light = false;
        bool show_unit_cylinder = false;

        bool gamma_correction = false;
        bool show_mesh_normals = false;
        bool show_floor = true;

        // coordinates tab state
        char coords_tab_filter[64]{};
        std::vector<osim::Coordinate> model_coords_swap;
        bool sort_coords_by_name = true;
        bool show_rotational_coords = true;
        bool show_translational_coords = true;
        bool show_coupled_coords = true;

        // model tab stuff
        std::vector<osim::Muscle_stat> model_muscs_swap;
        char model_tab_filter[64]{};

        // MAs tab state
        std::string const* mas_muscle_selection = nullptr;
        std::string const* mas_coord_selection = nullptr;
        static constexpr unsigned num_steps = 50;
        struct MA_plot final {
            std::string muscle_name;
            std::string coord_name;
            float x_begin;
            float x_end;
            std::array<float, num_steps> y_vals;
            float min;
            float max;
        };
        std::vector<std::unique_ptr<MA_plot>> mas_plots;

        sdl::Window_dimensions window_dims;

        std::optional<Background_fd_simulation> simulator;

        osim::OSMV_Model model;
        osim::State_geometry geom;
        osim::Geometry_loader geom_loader;
        osim::OSMV_State latest_state;
        std::unordered_map<osim::Mesh_id, Scene_mesh> meshes;

        Show_model_screen_impl(std::string _path, osim::OSMV_Model model);

        void init(Application&);
        osmv::Screen_response handle_event(Application&, SDL_Event&);
        Screen_response tick(Application&);

        void on_user_edited_model() {
            if (simulator) {
                simulator->request_stop();
                simulator->try_pop_latest(latest_state);
            }

            SimTK::State& s = osim::init_system(model);
            latest_state = s;

            mas_muscle_selection = nullptr;
            mas_coord_selection = nullptr;

            update_scene();
        }

        void on_user_edited_state() {
            simulator and simulator->request_stop();
            update_scene();
        }

        void update_scene();

        [[nodiscard]] bool simulator_running() const noexcept {
            return simulator and simulator->running();
        }

        void draw(Application&);
        void draw_model_scene(Application&);
        void draw_imgui_ui(Application&);
        void draw_menu_bar();
        void draw_lhs_panel(Application&);
        void draw_simulate_tab();
        void draw_ui_tab(Application&);
        void draw_coords_tab();
        void draw_coordinate_slider(osim::Coordinate const&);
        void draw_utils_tab();
        void draw_muscles_tab();
        void draw_mas_tab();
        void draw_outputs_tab();
    };
}

osmv::Show_model_screen::Show_model_screen(std::string path, osim::OSMV_Model model) :
    impl{new Show_model_screen_impl{std::move(path), std::move(model)}} {
}

osmv::Show_model_screen_impl::Show_model_screen_impl(std::string _path, osim::OSMV_Model _model) :
    path{std::move(_path)},
    model{std::move(_model)},
    latest_state{[this]() {
        osim::finalize_from_properties(model);
        return osim::OSMV_State{osim::init_system(model)};
    }()}
{
    update_scene();
}

void osmv::Show_model_screen_impl::update_scene() {
    osim::realize_velocity(model, latest_state);
    geom.clear();
    geom_loader.all_geometry_in(model, latest_state, geom);

    // populate mesh data, if necessary
    for (osim::Mesh_instance const& mi : geom.mesh_instances) {
        if (meshes.find(mi.mesh) == meshes.end()) {
            geom_loader.load_mesh(mi.mesh, mesh_swap_space);
            Scene_mesh m = upload_to_gpu(ucgs, sns, mesh_swap_space);
            meshes.emplace(mi.mesh, std::move(m));
        }
    }
}

osmv::Show_model_screen::~Show_model_screen() noexcept = default;

void osmv::Show_model_screen::init(osmv::Application& s) {
    impl->init(s);
}

void osmv::Show_model_screen_impl::init(Application & s) {
    window_dims = s.window_size();
}

osmv::Screen_response osmv::Show_model_screen::handle_event(Application& s, SDL_Event& e) {
    return impl->handle_event(s, e);
}

osmv::Screen_response osmv::Show_model_screen_impl::handle_event(Application& ui, SDL_Event& e) {
    ImGuiIO& io = ImGui::GetIO();
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
                    osim::realize_velocity(model, latest_state);
                    update_scene();
                }
                break;
            }
            case SDLK_SPACE: {
                if (simulator and simulator->running()) {
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

osmv::Screen_response osmv::Show_model_screen::tick(Application& a) {
    return impl->tick(a);
}

osmv::Screen_response osmv::Show_model_screen_impl::tick(Application &) {
    if (simulator and simulator->try_pop_latest(latest_state)) {
        osim::realize_report(model, latest_state);
        update_scene();
    }

    return Resp_Ok{};
}

void osmv::Show_model_screen::draw(osmv::Application& ui) {
    impl->draw(ui);
}

void osmv::Show_model_screen_impl::draw(osmv::Application& ui) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);

    if (gamma_correction) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    draw_model_scene(ui);
    draw_imgui_ui(ui);
}

// draw main 3D scene
void osmv::Show_model_screen_impl::draw_model_scene(Application& ui) {
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
        float aspect_ratio = static_cast<float>(window_dims.w) / static_cast<float>(window_dims.h);
        return glm::perspective(fov, aspect_ratio, 0.1f, 100.0f);
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
        gl::UseProgram(ucgs.program);

        gl::Uniform(ucgs.projMat, proj_mtx);
        gl::Uniform(ucgs.viewMat, view_mtx);
        gl::Uniform(ucgs.light_pos, light_pos);
        gl::Uniform(ucgs.light_color, glm::vec3(light_color[0], light_color[1], light_color[2]));
        gl::Uniform(ucgs.view_pos, view_pos);

        // draw model meshes
        for (auto& m : geom.mesh_instances) {
            gl::Uniform(ucgs.rgba, m.rgba);
            gl::Uniform(ucgs.modelMat, m.transform);
            gl::Uniform(ucgs.normalMat, m.normal_xform);

            Scene_mesh& md = asserting_find(meshes, m.mesh);
            gl::BindVertexArray(md.main_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, md.vbo.sizei());
            gl::BindVertexArray();
        }

        // debugging: draw unit cylinder
        if (show_unit_cylinder) {
            gl::BindVertexArray(cylinder.main_vao);

            gl::Uniform(ucgs.rgba, glm::vec4{0.9f, 0.9f, 0.9f, 1.0f});
            gl::Uniform(ucgs.modelMat, glm::identity<glm::mat4>());
            gl::Uniform(ucgs.normalMat, glm::identity<glm::mat4>());

            gl::DrawArrays(GL_TRIANGLES, 0, cylinder.vbo.sizei());

            gl::BindVertexArray();
        }

        // debugging: draw light location
        if (show_light) {
            gl::BindVertexArray(sphere.main_vao);

            gl::Uniform(ucgs.rgba, glm::vec4{1.0f, 1.0f, 0.0f, 0.3f});
            auto xform = glm::scale(glm::translate(glm::identity<glm::mat4>(), light_pos), {0.05, 0.05, 0.05});
            gl::Uniform(ucgs.modelMat, xform);
            gl::Uniform(ucgs.normalMat, glm::transpose(glm::inverse(xform)));
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
        gl::UseProgram(sns.program);
        gl::Uniform(sns.projMat, proj_mtx);
        gl::Uniform(sns.viewMat, view_mtx);

        for (auto& m : geom.mesh_instances) {
            gl::Uniform(sns.modelMat, m.transform);
            gl::Uniform(sns.normalMat, m.normal_xform);

            Scene_mesh& md = asserting_find(meshes, m.mesh);
            gl::BindVertexArray(md.normal_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, md.vbo.sizei());
            gl::BindVertexArray();
        }
    }
}

void osmv::Show_model_screen_impl::draw_imgui_ui(Application& ui) {
    //ImGui_ImplOpenGL3_NewFrame();
    //ImGui_ImplSDL2_NewFrame(ui.window);
    //ImGui::NewFrame();

    draw_menu_bar();
    draw_lhs_panel(ui);

    //ImGui::Render();
    //ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

void osmv::Show_model_screen_impl::draw_lhs_panel(Application& ui) {
    bool b = true;
    ImGuiWindowFlags flags = 0;
    ImGui::Begin("Model", &b, flags);

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

        if (ImGui::BeginTabItem("UI")) {
            ImGui::Dummy(ImVec2{0.0f, 5.0f});
            draw_ui_tab(ui);
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
        osim::realize_velocity(model, latest_state);
        update_scene();
    }

    ImGui::Dummy(ImVec2{0.0f, 20.0f});

    ImGui::Text("simulation config");
    ImGui::Separator();
    ImGui::SliderFloat("final time", &fd_final_time, 0.01f, 20.0f);

    if (simulator) {
        std::chrono::milliseconds wall_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(simulator->wall_duration());
        double wall_secs = static_cast<double>(wall_ms.count())/1000.0;
        double sim_secs = simulator->current_sim_time();
        double pct_completed = sim_secs/simulator->sim_final_time() * 100.0;

        ImGui::Dummy(ImVec2{0.0f, 20.0f});
        ImGui::Text("simulator stats");
        ImGui::Separator();
        ImGui::Text("status: %s", simulator->status_str());
        ImGui::Text("progress: %.2f %%", pct_completed);
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        ImGui::Text("simulation time: %.2f", sim_secs);
        ImGui::Text("wall time: %.2f secs", wall_secs);
        ImGui::Text("sim_time/wall_time: %.4f", sim_secs/wall_secs);
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        ImGui::Text("prescribeQ calls: %i", simulator->prescribeq_calls());
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        ImGui::Text("States sent to UI thread: %i", simulator->states_sent_to_ui());
        ImGui::Text("Avg. UI overhead: %.5f %%", 100.0 * simulator->ui_overhead());
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
    ImGui::ColorEdit3("light_color", light_color);
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
    ImGui::InputText("search", coords_tab_filter, sizeof(coords_tab_filter));
    ImGui::SameLine();
    if (std::strlen(coords_tab_filter) > 0 and ImGui::Button("clear")) {
        coords_tab_filter[0] = '\0';
    }

    ImGui::Dummy(ImVec2{0.0f, 5.0f});

    ImGui::Checkbox("sort", &sort_coords_by_name);
    ImGui::SameLine();
    ImGui::Checkbox("rotational", &show_rotational_coords);
    ImGui::SameLine();
    ImGui::Checkbox("translational", &show_translational_coords);

    ImGui::Checkbox("coupled", &show_coupled_coords);

    // get coords

    model_coords_swap.clear();
    osim::get_coordinates(model, latest_state, model_coords_swap);


    // apply filters etc.

    int coordtypes_to_filter_out = 0;
    if (not show_rotational_coords) {
        coordtypes_to_filter_out |= osim::Rotational;
    }
    if (not show_translational_coords) {
        coordtypes_to_filter_out |= osim::Translational;
    }
    if (not show_coupled_coords) {
        coordtypes_to_filter_out |= osim::Coupled;
    }

    auto it = std::remove_if(model_coords_swap.begin(), model_coords_swap.end(), [&](osim::Coordinate const& c) {
        if (c.type & coordtypes_to_filter_out) {
            return true;
        }

        if (c.name->find(coords_tab_filter) == c.name->npos) {
            return true;
        }

        return false;
    });
    model_coords_swap.erase(it, model_coords_swap.end());

    if (sort_coords_by_name) {
        std::sort(model_coords_swap.begin(), model_coords_swap.end(), [](osim::Coordinate const& c1, osim::Coordinate const& c2) {
            return *c1.name < *c2.name;
        });
    }


    // render sliders

    ImGui::Dummy(ImVec2{0.0f, 10.0f});
    ImGui::Text("Coordinates (%i)", static_cast<int>(model_coords_swap.size()));
    ImGui::Separator();

    int i = 0;
    for (osim::Coordinate const& c : model_coords_swap) {
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
    model_muscs_swap.clear();
    osim::get_muscle_stats(model, latest_state, model_muscs_swap);

    // sort muscles alphabetically by name
    std::sort(model_muscs_swap.begin(), model_muscs_swap.end(), [](osim::Muscle_stat const& m1, osim::Muscle_stat const& m2) {
        return *m1.name < *m2.name;
    });

    // allow user filtering
    ImGui::InputText("filter muscles", model_tab_filter, sizeof(model_tab_filter));
    ImGui::Separator();

    // draw muscle list
    for (osim::Muscle_stat const& musc : model_muscs_swap) {
        if (musc.name->find(model_tab_filter) != musc.name->npos) {
            ImGui::Text("%s (len = %.2f)", musc.name->c_str(), musc.length);
        }
    }
}

void osmv::Show_model_screen_impl::draw_mas_tab() {
    ImGui::Text("Moment arms");
    ImGui::Separator();

    ImGui::Columns(2);
    // lhs: muscle selection
    {
        model_muscs_swap.clear();
        osim::get_muscle_stats(model, latest_state, model_muscs_swap);

        // usability: sort by name
        std::sort(model_muscs_swap.begin(), model_muscs_swap.end(), [](osim::Muscle_stat const& m1, osim::Muscle_stat const& m2) {
            return *m1.name < *m2.name;
        });

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("MomentArmPlotMuscleSelection", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260), false, window_flags);
        for (osim::Muscle_stat const& m : model_muscs_swap) {
            if (ImGui::Selectable(m.name->c_str(), m.name == mas_muscle_selection)) {
                mas_muscle_selection = m.name;
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();
    }
    // rhs: coord selection
    {
        model_coords_swap.clear();
        osim::get_coordinates(model, latest_state, model_coords_swap);

        // usability: sort by name
        std::sort(model_coords_swap.begin(), model_coords_swap.end(), [](osim::Coordinate const& c1, osim::Coordinate const& c2) {
            return *c1.name < *c2.name;
        });

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("MomentArmPlotCoordSelection", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260), false, window_flags);
        for (osim::Coordinate const& c : model_coords_swap) {
            if (ImGui::Selectable(c.name->c_str(), c.name == mas_coord_selection)) {
                mas_coord_selection = c.name;
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();
    }
    ImGui::Columns();

    if (mas_muscle_selection and mas_coord_selection) {
        if (ImGui::Button("+ add plot")) {
            auto it = std::find_if(model_muscs_swap.begin(),
                                   model_muscs_swap.end(),
                                   [this](osim::Muscle_stat const& ms) {
                    return ms.name == mas_muscle_selection;
            });
            assert(it != model_muscs_swap.end());

            auto it2 = std::find_if(model_coords_swap.begin(),
                                    model_coords_swap.end(),
                                    [this](osim::Coordinate const& c) {
                    return c.name == mas_coord_selection;
            });
            assert(it2 != model_coords_swap.end());

            auto p = std::make_unique<MA_plot>();
            p->muscle_name = *mas_muscle_selection;
            p->coord_name = *mas_coord_selection;
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
            mas_coord_selection = nullptr;

            mas_plots.push_back(std::move(p));
        }
    }

    if (ImGui::Button("refresh TODO")) {
        // iterate through each plot in plots vector and recompute the moment
        // arms from the UI's current model + state
        throw std::runtime_error{"refreshing moment arm plots NYI"};
    }

    if (not mas_plots.empty() and ImGui::Button("clear all")) {
        mas_plots.clear();
    }

    ImGui::Separator();

    ImGui::Columns(2);
    for (size_t i = 0; i < mas_plots.size(); ++i) {
        MA_plot const& p = *mas_plots[i];
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
            auto it = mas_plots.begin() + i;
            mas_plots.erase(it, it + 1);
        }
        ImGui::NextColumn();
    }
    ImGui::Columns();
}

void osmv::Show_model_screen_impl::draw_outputs_tab() {
    static std::vector<std::string const*> swap;
    swap.clear();
    osim::get_output_vals(model, latest_state, swap);
    for (std::string const* name : swap) {
        ImGui::Text("%s", name->c_str());//
    }
}
