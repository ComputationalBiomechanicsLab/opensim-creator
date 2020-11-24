#include "show_model_screen.hpp"

#include "osmv_config.hpp"

#include "application.hpp"
#include "screen.hpp"
#include "meshes.hpp"
#include "opensim_wrapper.hpp"
#include "loading_screen.hpp"

// OpenGL
#include "gl.hpp"
#include "gl_extensions.hpp"

// glm
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

// imgui
#include "imgui.h"
#include "imgui_extensions.hpp"
#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"

// sdl
#include "sdl.hpp"

// c++
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <unordered_map>

constexpr float pi_f = static_cast<float>(M_PI);

// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace {
    using osmv::Mesh_point;
    using osmv::Vec3;

    // returns a procedurally-generated chequered floor texture
    gl::Texture_2d generate_chequered_floor() {
        struct Rgb { unsigned char r, g, b; };
        constexpr size_t w = 512;
        constexpr size_t h = 512;
        constexpr Rgb on_color = {0xfd, 0xfd, 0xfd};
        constexpr Rgb off_color = {0xeb, 0xeb, 0xeb};

        std::array<Rgb, w*h> pixels;
        for (size_t row = 0; row < h; ++row) {
            size_t row_start = row * w;
            bool y_on = (row/32) % 2 == 0;
            for (size_t col = 0; col < w; ++col) {
                bool x_on = (col/32) % 2 == 0;
                pixels[row_start + col] = y_on xor x_on ? on_color : off_color;
            }
        }

        gl::Texture_2d rv = gl::GenTexture2d();
        gl::BindTexture(rv.type, rv);
        glTexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        glGenerateMipmap(rv.type);
        return rv;
    }

    struct Floor_program final {
        gl::Program p = gl::CreateProgramFrom(
            gl::CompileVertexShaderFile(OSMV_SHADERS_DIR "floor.vert"),
            gl::CompileFragmentShaderFile(OSMV_SHADERS_DIR "floor.frag"));
        gl::Texture_2d tex = generate_chequered_floor();
        gl::Uniform_mat4f projMat = gl::GetUniformLocation(p, "projMat");
        gl::Uniform_mat4f viewMat = gl::GetUniformLocation(p, "viewMat");
        gl::Uniform_mat4f modelMat = gl::GetUniformLocation(p, "modelMat");
        gl::Uniform_1i uSampler0 = gl::GetUniformLocation(p, "uSampler0");
        static constexpr gl::Attribute aPos = 0;
        static constexpr gl::Attribute aTexCoord = 1;

        // instance stuff
        gl::Array_buffer quad_buf = []() {
            static const float vals[] = {
                // location         // tex coords
                 1.0f,  1.0f, 0.0f,   100.0f, 100.0f,
                 1.0f, -1.0f, 0.0f,   100.0f,  0.0f,
                -1.0f, -1.0f, 0.0f,    0.0f,  0.0f,

                -1.0f, -1.0f, 0.0f,    0.0f, 0.0f,
                -1.0f,  1.0f, 0.0f,    0.0f, 100.0f,
                 1.0f,  1.0f, 0.0f,   100.0f, 100.0f,
            };

            auto buf = gl::GenArrayBuffer();
            gl::BindBuffer(buf.type, buf);
            gl::BufferData(buf.type, sizeof(vals), vals, GL_STATIC_DRAW);
            return buf;
        }();

        gl::Vertex_array vao = [&]() {
            auto vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::BindBuffer(quad_buf.type, quad_buf);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), nullptr);
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();
            return vao;
        }();

        glm::mat4 model_mtx = glm::scale(glm::rotate(glm::identity<glm::mat4>(), pi_f/2, {1.0, 0.0, 0.0}), {100.0f, 100.0f, 0.0f});
    };

    // represents a vbo containing some verts /w normals
    struct Vbo_Triangles_with_norms final {
        GLsizei num_verts = 0;
        gl::Array_buffer vbo = gl::GenArrayBuffer();

        Vbo_Triangles_with_norms(std::vector<Mesh_point> const& points)
            : num_verts(static_cast<GLsizei>(points.size())) {

            gl::BindBuffer(vbo.type, vbo);
            gl::BufferData(vbo.type, sizeof(Mesh_point) * points.size(), points.data(), GL_STATIC_DRAW);
        }
    };

    // basic GL program that just renders mesh normals: useful for debugging
    struct Normals_program final {
        gl::Program program = gl::CreateProgramFrom(
            gl::CompileVertexShaderFile(OSMV_SHADERS_DIR "normals.vert"),
            gl::CompileFragmentShaderFile(OSMV_SHADERS_DIR "normals.frag"),
            gl::CompileGeometryShaderFile(OSMV_SHADERS_DIR "normals.geom"));

        gl::Uniform_mat4f projMat = gl::GetUniformLocation(program, "projMat");
        gl::Uniform_mat4f viewMat = gl::GetUniformLocation(program, "viewMat");
        gl::Uniform_mat4f modelMat = gl::GetUniformLocation(program, "modelMat");
        gl::Uniform_mat4f normalMat = gl::GetUniformLocation(program, "normalMat");
        static constexpr gl::Attribute aPos = 0;
        static constexpr gl::Attribute aNormal = 1;
    };

    // a (potentially shared) mesh that is ready for `Normals_program` to render
    struct Normals_program_renderable final {
        std::shared_ptr<Vbo_Triangles_with_norms> verts;
        gl::Vertex_array vao;

        Normals_program_renderable(Normals_program& p,
                                   std::shared_ptr<Vbo_Triangles_with_norms> _verts) :
            verts{std::move(_verts)},
            vao{[&]() {
                auto vao = gl::GenVertexArrays();
                gl::BindVertexArray(vao);
                gl::BindBuffer(verts->vbo.type, verts->vbo);
                gl::VertexAttribPointer(p.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_point), 0);
                gl::EnableVertexAttribArray(p.aPos);
                gl::VertexAttribPointer(p.aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_point), (void*)sizeof(Vec3));
                gl::EnableVertexAttribArray(p.aNormal);
                gl::BindVertexArray();
                return vao;
            }()} {
        }
    };

    // main program for rendering: basic scene /w no materials
    struct Main_program final {
        gl::Program program = gl::CreateProgramFrom(
            gl::CompileVertexShaderFile(OSMV_SHADERS_DIR "main.vert"),
            gl::CompileFragmentShaderFile(OSMV_SHADERS_DIR "main.frag"));

        gl::Uniform_mat4f projMat = gl::GetUniformLocation(program, "projMat");
        gl::Uniform_mat4f viewMat = gl::GetUniformLocation(program, "viewMat");
        gl::Uniform_mat4f modelMat = gl::GetUniformLocation(program, "modelMat");
        gl::Uniform_mat4f normalMat = gl::GetUniformLocation(program, "normalMat");
        gl::Uniform_vec4f rgba = gl::GetUniformLocation(program, "rgba");
        gl::Uniform_vec3f light_pos = gl::GetUniformLocation(program, "lightPos");
        gl::Uniform_vec3f light_color = gl::GetUniformLocation(program, "lightColor");
        gl::Uniform_vec3f view_pos = gl::GetUniformLocation(program, "viewPos");

        gl::Attribute location = gl::GetAttribLocation(program, "location");
        gl::Attribute in_normal = gl::GetAttribLocation(program, "in_normal");
    };

    // A (potentially shared) mesh that can be rendered by `Main_program`
    struct Main_program_renderable final {
        std::shared_ptr<Vbo_Triangles_with_norms> verts;
        gl::Vertex_array vao;

        Main_program_renderable(Main_program& p,
                                std::shared_ptr<Vbo_Triangles_with_norms> _verts) :
            verts{std::move(_verts)},
            vao{[&]() {
                auto vao = gl::GenVertexArrays();
                gl::BindVertexArray(vao);
                {
                    gl::BindBuffer(verts->vbo.type, verts->vbo);
                    gl::VertexAttribPointer(p.location, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_point), 0);
                    gl::EnableVertexAttribArray(p.location);
                    gl::VertexAttribPointer(p.in_normal, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_point), (void*)sizeof(Vec3));
                    gl::EnableVertexAttribArray(p.in_normal);
                    gl::BindVertexArray();
                }
                return vao;
            }()} {
        }
    };

    // create an xform that transforms the unit cylinder into a line between
    // two points
    glm::mat4 cylinder_to_line_xform(float line_width,
                                     glm::vec3 const& p1,
                                     glm::vec3 const& p2) {
        glm::vec3 p1_to_p2 = p2 - p1;
        glm::vec3 c1_to_c2 = glm::vec3{0.0f, 2.0f, 0.0f};
        auto rotation =
                glm::rotate(glm::identity<glm::mat4>(),
                            glm::acos(glm::dot(glm::normalize(c1_to_c2), glm::normalize(p1_to_p2))),
                            glm::cross(glm::normalize(c1_to_c2), glm::normalize(p1_to_p2)));
        float scale = glm::length(p1_to_p2)/glm::length(c1_to_c2);
        auto scale_xform = glm::scale(glm::identity<glm::mat4>(), glm::vec3{line_width, scale, line_width});
        auto translation = glm::translate(glm::identity<glm::mat4>(), p1 + p1_to_p2/2.0f);
        return translation * rotation * scale_xform;
    }

    std::tuple<Mesh_point, Mesh_point, Mesh_point> to_mesh_points(osim::Triangle const& t) {
        glm::vec3 normal = glm::cross((t.p2 - t.p1), (t.p3 - t.p1));
        Vec3 normal_vec3 = Vec3{normal.x, normal.y, normal.z};

        return {
            Mesh_point{Vec3{t.p1.x, t.p1.y, t.p1.z}, normal_vec3},
            Mesh_point{Vec3{t.p2.x, t.p2.y, t.p2.z}, normal_vec3},
            Mesh_point{Vec3{t.p3.x, t.p3.y, t.p3.z}, normal_vec3}
        };
    }

    // TODO: this is hacked together
    Vbo_Triangles_with_norms make_mesh(osim::Mesh const& data) {
        std::vector<Mesh_point> triangles;
        triangles.reserve(3*data.triangles.size());
        for (osim::Triangle const& t : data.triangles) {
            auto [p1, p2, p3] = to_mesh_points(t);
            triangles.emplace_back(p1);
            triangles.emplace_back(p2);
            triangles.emplace_back(p3);
        }
        return Vbo_Triangles_with_norms{triangles};
    }

    // mesh received from OpenSim model and then uploaded into OpenGL
    struct Osim_mesh final {
        Main_program_renderable main_prog_renderable;
        Normals_program_renderable normals_prog_renderable;

        Osim_mesh(Main_program& mp, Normals_program& np, osim::Mesh const& m) :
            main_prog_renderable{mp, std::make_shared<Vbo_Triangles_with_norms>(make_mesh(m))},
            normals_prog_renderable{np, main_prog_renderable.verts} {
        }
    };
}

namespace osmv {
    struct Show_model_screen_impl final {
        std::string path;

        Main_program pMain = {};
        Normals_program pNormals = {};

        Main_program_renderable pMain_cylinder =
            {pMain, std::make_shared<Vbo_Triangles_with_norms>(osmv::simbody_cylinder_triangles(12))};
        Normals_program_renderable pNormals_cylinder =
            {pNormals, pMain_cylinder.verts};

        Main_program_renderable pMain_sphere =
            {pMain, std::make_shared<Vbo_Triangles_with_norms>(osmv::unit_sphere_triangles())};
        Main_program_renderable pNormals_sphere =
            {pMain, pMain_sphere.verts};

        Floor_program pFloor;

        bool wireframe_mode = false;
        float radius = 1.0f;
        float wheel_sensitivity = 0.9f;

        float fov = 120.0f;

        bool dragging = false;
        float theta = 0.0f;
        float phi = 0.0f;
        float sensitivity = 1.0f;

        bool panning = false;
        glm::vec3 pan = {0.0f, 0.0f, 0.0f};

        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        float light_color[3] = {0.9607f, 0.9176f, 0.8863f};
        bool show_light = false;
        bool show_unit_cylinder = false;

        bool gamma_correction = false;
        bool user_gamma_correction = gamma_correction;
        bool show_cylinder_normals = false;
        bool show_sphere_normals = false;
        bool show_mesh_normals = false;

        sdl::Window_dimensions window_dims;

        osim::Model_wrapper model;
        osim::State_wrapper state;
        osim::State_geometry geom;
        osim::Geometry_loader geom_loader;

        osim::Mesh mesh_swap_space;
        osim::Mesh_loader mesh_loader;
        std::unordered_map<osim::Mesh_id, Osim_mesh> meshes;

        Show_model_screen_impl(std::string _path, osim::Model_wrapper model);

        void init(Application&);
        osmv::Screen_response handle_event(Application&, SDL_Event&);
        void draw(Application&);
    };
}

osmv::Show_model_screen::Show_model_screen(std::string path, osim::Model_wrapper model) :
    impl{new Show_model_screen_impl{std::move(path), std::move(model)}} {
}

osmv::Show_model_screen_impl::Show_model_screen_impl(std::string _path, osim::Model_wrapper _model) :
    path{std::move(_path)},
    model{std::move(_model)},
    state{osim::initial_state(model)} {

    // populate geometry
    geom_loader.geometry_in(state, geom);

    for (osim::Mesh_instance const& mi : geom.mesh_instances) {
        std::string const& path = geom_loader.path_to(mi.mesh);
        mesh_loader.load(path, mesh_swap_space);
        meshes.emplace(mi.mesh, Osim_mesh{pMain, pNormals, mesh_swap_space});
    }

    // HACK: xform lines to cylinders
    for (osim::Line const& l : geom.lines) {
        glm::mat4 transform = cylinder_to_line_xform(0.005, l.p1, l.p2);
        osim::Cylinder c{transform, glm::transpose(glm::inverse(transform)), l.rgba};
        geom.cylinders.emplace_back(c);
    }
}

osmv::Show_model_screen::~Show_model_screen() noexcept = default;

void osmv::Show_model_screen::init(osmv::Application& s) {
    impl->init(s);
}

void osmv::Show_model_screen_impl::init(Application & s) {
    if (gamma_correction) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }
    window_dims = sdl::GetWindowSize(s.window);
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
        case SDLK_r:
            return Resp_Transition_to{std::make_unique<osmv::Loading_screen>(path)};
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
                SDL_WarpMouseInWindow(ui.window, edge_width, e.motion.y);
            }
            if (e.motion.x - edge_width < 0) {
                SDL_WarpMouseInWindow(ui.window, window_dims.w - edge_width, e.motion.y);
            }
            if (e.motion.y + edge_width > window_dims.h) {
                SDL_WarpMouseInWindow(ui.window, e.motion.x, edge_width);
            }
            if (e.motion.y - edge_width < 0) {
                SDL_WarpMouseInWindow(ui.window, e.motion.x, window_dims.h - edge_width);
            }
        }
    } else if (e.type == SDL_WINDOWEVENT) {
        window_dims = sdl::GetWindowSize(ui.window);
        glViewport(0, 0, window_dims.w, window_dims.h);
    } else if (e.type == SDL_MOUSEWHEEL) {
        if (e.wheel.y > 0 and radius >= 0.1f) {
            radius *= wheel_sensitivity;
        }

        if (e.wheel.y <= 0 and radius < 100.0f) {
            radius /= wheel_sensitivity;
        }
    }

    return Resp_Ok{};
}

void osmv::Show_model_screen::draw(osmv::Application& ui) {
    impl->draw(ui);
}

template<typename K, typename V>
static V& throwing_find(std::unordered_map<K, V>& m, K const& k) {
    auto it = m.find(k);
    if (it != m.end()) {
        return it->second;
    }
    std::stringstream ss;
    ss << k << ": not found in mesh map lookup: was it loaded?";
    throw std::runtime_error{ss.str()};
}

void osmv::Show_model_screen_impl::draw(osmv::Application& ui) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);

    if (user_gamma_correction != gamma_correction) {
        if (user_gamma_correction) {
            glEnable(GL_FRAMEBUFFER_SRGB);
        } else {
            glDisable(GL_FRAMEBUFFER_SRGB);
        }
        gamma_correction = user_gamma_correction;
    }

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


    // PROGRAM: draw main scene
    gl::UseProgram(pMain.program);

    gl::Uniform(pMain.projMat, proj_mtx);
    gl::Uniform(pMain.viewMat, view_mtx);
    gl::Uniform(pMain.light_pos, light_pos);
    gl::Uniform(pMain.light_color, glm::vec3(light_color[0], light_color[1], light_color[2]));
    gl::Uniform(pMain.view_pos, view_pos);

    // draw cylinders
    {
        auto num_verts = pMain_cylinder.verts->num_verts;

        gl::BindVertexArray(pMain_cylinder.vao);

        for (auto const& c : geom.cylinders) {
            gl::Uniform(pMain.rgba, c.rgba);
            gl::Uniform(pMain.modelMat, c.transform);
            gl::Uniform(pMain.normalMat, c.normal_xform);
            glDrawArrays(GL_TRIANGLES, 0, num_verts);
        }

        if (show_unit_cylinder) {
            gl::Uniform(pMain.rgba, glm::vec4{0.9f, 0.9f, 0.9f, 1.0f});
            gl::Uniform(pMain.modelMat, glm::identity<glm::mat4>());
            gl::Uniform(pMain.normalMat, glm::identity<glm::mat4>());
            glDrawArrays(GL_TRIANGLES, 0, num_verts);
        }

        gl::BindVertexArray();
    }

    // draw spheres
    {
        auto num_verts = pMain_sphere.verts->num_verts;

        gl::BindVertexArray(pMain_sphere.vao);

        for (auto const& c : geom.spheres) {
            gl::Uniform(pMain.rgba, c.rgba);
            gl::Uniform(pMain.modelMat, c.transform);
            gl::Uniform(pMain.normalMat, c.normal_xform);
            glDrawArrays(GL_TRIANGLES, 0, num_verts);
        }

        if (show_light) {
            gl::Uniform(pMain.rgba, glm::vec4{1.0f, 1.0f, 0.0f, 0.3f});
            auto xform = glm::scale(glm::translate(glm::identity<glm::mat4>(), light_pos), {0.05, 0.05, 0.05});
            gl::Uniform(pMain.modelMat, xform);
            gl::Uniform(pMain.normalMat, glm::transpose(glm::inverse(xform)));
            glDrawArrays(GL_TRIANGLES, 0, num_verts);
        }

        gl::BindVertexArray();
    }

    // draw meshes
    for (auto& m : geom.mesh_instances) {
        gl::Uniform(pMain.rgba, m.rgba);
        gl::Uniform(pMain.modelMat, m.transform);
        gl::Uniform(pMain.normalMat, m.normal_xform);

        Osim_mesh& md = throwing_find(meshes, m.mesh);
        gl::BindVertexArray(md.main_prog_renderable.vao);
        glDrawArrays(GL_TRIANGLES, 0, md.main_prog_renderable.verts->num_verts);
        gl::BindVertexArray();
    }

    // PROGRAM: draw scene normals (if required)
    if (show_sphere_normals or show_cylinder_normals or show_mesh_normals) {
        gl::UseProgram(pNormals.program);
        gl::Uniform(pNormals.projMat, proj_mtx);
        gl::Uniform(pNormals.viewMat, view_mtx);

        if (show_cylinder_normals) {
            auto num_verts = pMain_cylinder.verts->num_verts;

            gl::BindVertexArray(pNormals_cylinder.vao);

            for (auto const& c : geom.cylinders) {
                gl::Uniform(pNormals.modelMat, c.transform);
                gl::Uniform(pNormals.normalMat, c.normal_xform);
                glDrawArrays(GL_TRIANGLES, 0, num_verts);
            }

            if (show_unit_cylinder) {
                gl::Uniform(pNormals.modelMat, glm::identity<glm::mat4>());
                glDrawArrays(GL_TRIANGLES, 0, num_verts);
            }

            gl::BindVertexArray();
        }


        if (show_sphere_normals) {
            auto num_verts = pNormals_sphere.verts->num_verts;

            gl::BindVertexArray(pNormals_sphere.vao);

            for (auto const& c : geom.spheres) {
                gl::Uniform(pNormals.modelMat, c.transform);
                gl::Uniform(pNormals.normalMat, c.normal_xform);
                glDrawArrays(GL_TRIANGLES, 0, num_verts);
            }

            if (show_light) {
                gl::Uniform(pNormals.modelMat, glm::scale(glm::translate(glm::identity<glm::mat4>(), light_pos), {0.05, 0.05, 0.05}));
                glDrawArrays(GL_TRIANGLES, 0, num_verts);
            }

            gl::BindVertexArray();
        }

        if (show_mesh_normals) {
            for (auto& m : geom.mesh_instances) {
                gl::Uniform(pNormals.modelMat, m.transform);
                gl::Uniform(pNormals.normalMat, m.normal_xform);

                Osim_mesh& md = throwing_find(meshes, m.mesh);
                gl::BindVertexArray(md.normals_prog_renderable.vao);
                glDrawArrays(GL_TRIANGLES, 0, md.normals_prog_renderable.verts->num_verts);
                gl::BindVertexArray();
            }
        }
    }

    // PROGRAM: draw floor
    {
        gl::UseProgram(pFloor.p);
        gl::Uniform(pFloor.projMat, proj_mtx);
        gl::Uniform(pFloor.viewMat, view_mtx);
        gl::Uniform(pFloor.modelMat, pFloor.model_mtx);
        glActiveTexture(GL_TEXTURE0);
        gl::BindTexture(pFloor.tex.type, pFloor.tex);
        gl::Uniform(pFloor.uSampler0, 0);
        gl::BindVertexArray(pFloor.vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        gl::BindVertexArray();
    }

    // imgui: draw GUI on top of main scene
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(ui.window);
    ImGui::NewFrame();

    // frame: scene
    {
        bool b = true;
        ImGui::Begin("Scene", &b, ImGuiWindowFlags_MenuBar);
    }

    {
        ImGuiIO& io = ImGui::GetIO();
        std::stringstream fps;
        fps << "Fps: " << io.Framerate;
        ImGui::Text(std::move(fps).str().c_str());
    }
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
    ImGui::Checkbox("gamma_correction", &user_gamma_correction);
    ImGui::Checkbox("software_throttle", &ui.software_throttle);
    ImGui::Checkbox("show_cylinder_normals", &show_cylinder_normals);
    ImGui::Checkbox("show_sphere_normals", &show_sphere_normals);
    ImGui::Checkbox("show_mesh_normals", &show_mesh_normals);

    ImGui::NewLine();
    ImGui::Text("Interaction:");
    {
        std::string msgs;
        if (dragging) {
            msgs += "rotating ";
        }
        if (panning) {
            msgs += "panning ";
        }
        ImGui::Text(msgs.c_str());
    }
    ImGui::End();


    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
