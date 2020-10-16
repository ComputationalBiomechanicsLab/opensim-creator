#include "show_model_screen.hpp"

#include "osmv_config.hpp"

#include "application.hpp"
#include "screen.hpp"
#include "meshes.hpp"
#include "opensim_wrapper.hpp"

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
        constexpr Rgb on_color = {0xff, 0xff, 0xff};
        constexpr Rgb off_color = {0x00, 0x00, 0x00};

        std::array<Rgb, w*h> pixels;
        for (size_t row = 0; row < h; ++row) {
            size_t row_start = row * w;
            bool y_on = (row/16) % 2 == 0;
            for (size_t col = 0; col < w; ++col) {
                bool x_on = (col/16) % 2 == 0;

                if (y_on && x_on) {
                    pixels[row_start + col] = on_color;
                } else {
                    pixels[row_start + col] = off_color;
                }
            }
        }

        gl::Texture_2d rv;
        gl::BindTexture(rv);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        return rv;
    }

    struct Floor_program final {

    };

    // represents a vbo containing some verts /w normals
    struct Vbo_Triangles_with_norms final {
        GLsizei num_verts = 0;
        gl::Array_buffer vbo = {};

        Vbo_Triangles_with_norms(std::vector<Mesh_point> const& points)
            : num_verts(static_cast<GLsizei>(points.size())) {

            gl::BindBuffer(vbo);
            gl::BufferData(vbo, sizeof(Mesh_point) * points.size(), points.data(), GL_STATIC_DRAW);
        }
    };

    // basic GL program that just renders mesh normals: useful for debugging
    struct Normals_program final {
        gl::Program program = gl::CreateProgramFrom(
            gl::CompileVertexShaderFile(OSMV_SHADERS_DIR "normals.vert"),
            gl::CompileFragmentShaderFile(OSMV_SHADERS_DIR "normals.frag"),
            gl::CompileGeometryShaderFile(OSMV_SHADERS_DIR "normals.geom"));

        gl::UniformMatrix4fv projMat = gl::GetUniformLocation(program, "projMat");
        gl::UniformMatrix4fv viewMat = gl::GetUniformLocation(program, "viewMat");
        gl::UniformMatrix4fv modelMat = gl::GetUniformLocation(program, "modelMat");
        gl::UniformMatrix4fv normalMat = gl::GetUniformLocation(program, "normalMat");
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
                gl::BindBuffer(verts->vbo);
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

        gl::UniformMatrix4fv projMat = gl::GetUniformLocation(program, "projMat");
        gl::UniformMatrix4fv viewMat = gl::GetUniformLocation(program, "viewMat");
        gl::UniformMatrix4fv modelMat = gl::GetUniformLocation(program, "modelMat");
        gl::UniformMatrix4fv normalMat = gl::GetUniformLocation(program, "normalMat");
        gl::UniformVec4f rgba = gl::GetUniformLocation(program, "rgba");
        gl::UniformVec3f light_pos = gl::GetUniformLocation(program, "lightPos");
        gl::UniformVec3f light_color = gl::GetUniformLocation(program, "lightColor");
        gl::UniformVec3f view_pos = gl::GetUniformLocation(program, "viewPos");

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
                    gl::BindBuffer(verts->vbo);
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
    glm::mat4 cylinder_to_line_xform(glm::vec4 const& color,
                                     float line_width,
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
        glm::vec4 rgba;
        glm::mat4 transform;
        glm::mat4 normal_xform;
        Main_program_renderable main_prog_renderable;
        Normals_program_renderable normals_prog_renderable;

        Osim_mesh(Main_program& mp, Normals_program& np, osim::Mesh const& m) :
            rgba{m.rgba},
            transform{m.transform},
            normal_xform{m.normal_xform},
            main_prog_renderable{mp, std::make_shared<Vbo_Triangles_with_norms>(make_mesh(m))},
            normals_prog_renderable{np, main_prog_renderable.verts} {
        }
    };

    // geometry pulled out of an OpenSim model
    struct Model_geometry final {
        std::vector<osim::Cylinder> cylinders;
        std::vector<osim::Sphere> spheres;
        std::vector<Osim_mesh> meshes;
    };

    // helper function that visits an OpenSim model and pulls geometry out of
    // it
    Model_geometry load_model(Main_program& mp, Normals_program& np, std::vector<osim::Geometry> const & geometry) {
        Model_geometry rv;
        for (osim::Geometry const& g : geometry) {
            std::visit(overloaded {
                [&](osim::Cylinder const& c) {
                    rv.cylinders.push_back(c);
                },
                [&](osim::Line const& l) {
                    // HACK: lines are just cylinders, but transformed to look
                    // like lines. Do not use OpenGL's built-in lines because
                    // they suck.
                    float line_width = 0.0045f;
                    auto xform = cylinder_to_line_xform(l.rgba, line_width, l.p1, l.p2);
                    rv.cylinders.push_back(osim::Cylinder{xform, glm::transpose(glm::inverse(xform)), l.rgba});
                },
                [&](osim::Sphere const& s) {
                    rv.spheres.push_back(s);
                },
                [&](osim::Mesh const& m) {
                    rv.meshes.push_back(Osim_mesh{mp, np, m});
                }
            }, g);
        }
        return rv;
    }
}

namespace osmv {
    struct Show_model_screen_impl final {
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

        glm::vec3 light_pos = {1.0f, 1.0f, 0.0f};
        float light_color[3] = {0.9607f, 0.9176f, 0.8863f};
        bool show_light = false;
        bool show_unit_cylinder = false;

        bool gamma_correction = false;
        bool user_gamma_correction = gamma_correction;
        bool show_cylinder_normals = false;
        bool show_sphere_normals = false;
        bool show_mesh_normals = false;

        sdl::Window_dimensions window_dims;

        Model_geometry ms;  // loaded in ctor

        Show_model_screen_impl(std::vector<osim::Geometry> const& geometry);

        void init(Application&);
        osmv::Screen_response handle_event(Application&, SDL_Event&);
        void draw(Application&);
    };
}

osmv::Show_model_screen::Show_model_screen(std::vector<osim::Geometry> const & geometry)
    : impl{new Show_model_screen_impl{geometry}} {
}

osmv::Show_model_screen_impl::Show_model_screen_impl(std::vector<osim::Geometry> const& geometry) :
    ms{load_model(pMain, pNormals, geometry)} {

    if (gamma_correction) {
        OSC_GL_CALL_CHECK(glEnable, GL_FRAMEBUFFER_SRGB);
    } else {
        OSC_GL_CALL_CHECK(glDisable, GL_FRAMEBUFFER_SRGB);
    }
}

osmv::Show_model_screen::~Show_model_screen() noexcept = default;

void osmv::Show_model_screen::init(osmv::Application& s) {
    impl->init(s);
}

void osmv::Show_model_screen_impl::init(Application & s) {
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

void osmv::Show_model_screen_impl::draw(osmv::Application& ui) {
    ImGuiIO& io = ImGui::GetIO();

    sdl::Window_dimensions window_dims = sdl::GetWindowSize(ui.window);
    auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), -theta, glm::vec3{ 0.0f, 1.0f, 0.0f });
    auto theta_vec = glm::normalize(glm::vec3{ sin(theta), 0.0f, cos(theta) });
    auto phi_axis = glm::cross(theta_vec, glm::vec3{ 0.0, 1.0f, 0.0f });
    auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), -phi, phi_axis);
    auto pan_translate = glm::translate(glm::identity<glm::mat4>(), pan);
    float aspect_ratio = static_cast<float>(window_dims.w) / static_cast<float>(window_dims.h);

    if (user_gamma_correction != gamma_correction) {
        if (user_gamma_correction) {
            glEnable(GL_FRAMEBUFFER_SRGB);
        } else {
            glDisable(GL_FRAMEBUFFER_SRGB);
        }
        gamma_correction = user_gamma_correction;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);

    gl::UseProgram(pMain.program);

    // camera: at a fixed position pointing at a fixed origin. The "camera" works by translating +
    // rotating all objects around that origin. Rotation is expressed as polar coordinates. Camera
    // panning is represented as a translation vector.
    gl::Uniform(pMain.projMat, glm::perspective(fov, aspect_ratio, 0.1f, 100.0f));
    gl::Uniform(pMain.viewMat,
                glm::lookAt(
                    glm::vec3(0.0f, 0.0f, radius),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3{0.0f, 1.0f, 0.0f}) * rot_theta * rot_phi * pan_translate);
    gl::Uniform(pMain.light_pos, light_pos);
    gl::Uniform(pMain.light_color, glm::vec3(light_color[0], light_color[1], light_color[2]));
    gl::Uniform(pMain.view_pos, // polar-to-cartesian
                glm::vec3{radius * sin(theta) * cos(phi),
                          radius * sin(phi),
                          radius * cos(theta) * cos(phi)});

    // draw cylinders
    {
        auto num_verts = pMain_cylinder.verts->num_verts;

        gl::BindVertexArray(pMain_cylinder.vao);

        for (auto const& c : ms.cylinders) {
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

        for (auto const& c : ms.spheres) {
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

    // draw arbitrary meshes
    for (auto& m : ms.meshes) {
        gl::Uniform(pMain.rgba, m.rgba);
        gl::Uniform(pMain.modelMat, m.transform);
        gl::Uniform(pMain.normalMat, m.normal_xform);
        gl::BindVertexArray(m.main_prog_renderable.vao);
        glDrawArrays(GL_TRIANGLES, 0, m.main_prog_renderable.verts->num_verts);
        gl::BindVertexArray();
    }

    if (show_sphere_normals or show_cylinder_normals or show_mesh_normals) {
        gl::UseProgram(pNormals.program);
        gl::Uniform(pNormals.projMat, glm::perspective(fov, aspect_ratio, 0.1f, 100.0f));
        gl::Uniform(pNormals.viewMat,
                    glm::lookAt(
                        glm::vec3(0.0f, 0.0f, radius),
                        glm::vec3(0.0f, 0.0f, 0.0f),
                        glm::vec3{0.0f, 1.0f, 0.0f}) * rot_theta * rot_phi * pan_translate);

        if (show_cylinder_normals) {
            auto num_verts = pMain_cylinder.verts->num_verts;

            gl::BindVertexArray(pNormals_cylinder.vao);

            for (auto const& c : ms.cylinders) {
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

            for (auto const& c : ms.spheres) {
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
            for (auto& m : ms.meshes) {
                gl::Uniform(pNormals.modelMat, m.transform);
                gl::Uniform(pNormals.normalMat, m.normal_xform);
                gl::BindVertexArray(m.normals_prog_renderable.vao);
                glDrawArrays(GL_TRIANGLES, 0, m.normals_prog_renderable.verts->num_verts);
                gl::BindVertexArray();
            }
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(ui.window);

    ImGui::NewFrame();

    {
        bool b = true;
        ImGui::Begin("Scene", &b, ImGuiWindowFlags_MenuBar);
    }

    {
        std::stringstream fps;
        fps << "Fps: " << io.Framerate;
        ImGui::Text(fps.str().c_str());
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
        std::stringstream msgs;
        if (dragging) {
            msgs << "rotating ";
        }
        if (panning) {
            msgs << "panning ";
        }
        ImGui::Text(msgs.str().c_str());
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
