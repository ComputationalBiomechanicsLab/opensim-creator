#include "hittest_screen.hpp"

#include "src/app.hpp"
#include "src/log.hpp"
#include "src/3d/constants.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/3d/model.hpp"
#include "src/screens/experimental/experiments_screen.hpp"

#include <imgui.h>
#include <glm/vec3.hpp>

#include <array>

static char const g_VertexShader[] = R"(
    #version 330 core

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;

    layout (location = 0) in vec3 aPos;

    void main() {
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    }
)";

static char const g_FragmentShader[] = R"(
    #version 330 core

    uniform vec4 uColor;

    out vec4 FragColor;

    void main() {
        FragColor = uColor;
    }
)";

namespace {

    // basic shader that just colors the geometry in
    struct Shader final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::Vertex_shader>(g_VertexShader),
            gl::CompileFromSource<gl::Fragment_shader>(g_FragmentShader));

        gl::Attribute_vec3 aPos{0};

        gl::Uniform_mat4 uModel = gl::GetUniformLocation(prog, "uModel");
        gl::Uniform_mat4 uView = gl::GetUniformLocation(prog, "uView");
        gl::Uniform_mat4 uProjection = gl::GetUniformLocation(prog, "uProjection");
        gl::Uniform_vec4 uColor = gl::GetUniformLocation(prog, "uColor");
    };

    struct Scene_sphere final {
        glm::vec3 pos;
        bool is_hovered = false;

        Scene_sphere(glm::vec3 pos_) : pos{pos_} {
        }
    };
}

static constexpr std::array<glm::vec3, 4> g_CrosshairVerts = {{
    // -X to +X
    {-0.05f, 0.0f, 0.0f},
    {+0.05f, 0.0f, 0.0f},

    // -Y to +Y
    {0.0f, -0.05f, 0.0f},
    {0.0f, +0.05f, 0.0f}
}};

// make a VAO for the basic shader
static gl::Vertex_array make_vao(Shader& shader, gl::Array_buffer<glm::vec3>& vbo) {
    gl::Vertex_array rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(shader.aPos);
    gl::BindVertexArray();
    return rv;
}

static std::vector<Scene_sphere> generate_scene_spheres() {
    constexpr int min = -30;
    constexpr int max = 30;
    constexpr int step = 6;

    std::vector<Scene_sphere> rv;
    for (int x = min; x <= max; x += step) {
        for (int y = min; y <= max; y += step) {
            for (int z = min; z <= max; z += step) {
                rv.emplace_back(glm::vec3{x, 50.0f + 2.0f*y, z});
            }
        }
    }
    return rv;
}

// screen impl.
struct osc::Hittest_screen::Impl final {
    Shader shader;

    // sphere datas
    std::vector<glm::vec3> sphere_verts = gen_untextured_uv_sphere(12, 12).verts;
    AABB sphere_aabbs = aabb_from_points(sphere_verts.data(), sphere_verts.size());
    Sphere sphere_bound = sphere_bounds_of_points(sphere_verts.data(), sphere_verts.size());
    gl::Array_buffer<glm::vec3> sphere_vbo{sphere_verts};
    gl::Vertex_array sphere_vao = make_vao(shader, sphere_vbo);

    // sphere instances
    std::vector<Scene_sphere> spheres = generate_scene_spheres();

    // crosshair
    gl::Array_buffer<glm::vec3> crosshair_vbo{g_CrosshairVerts};
    gl::Vertex_array crosshair_vao = make_vao(shader, crosshair_vbo);

    // wireframe cube
    gl::Array_buffer<glm::vec3> cube_wireframe_vbo{gen_cube_lines().verts};
    gl::Vertex_array cube_wireframe_vao = make_vao(shader, cube_wireframe_vbo);

    // circle
    gl::Array_buffer<glm::vec3> circle_vbo{gen_circle(36).verts};
    gl::Vertex_array circle_vao = make_vao(shader, circle_vbo);

    // triangle
    std::array<glm::vec3, 3> triangle = {{
        {-10.0f, -10.0f, 0.0f},
        {+0.0f, +10.0f, 0.0f},
        {+10.0f, -10.0f, 0.0f},
    }};
    gl::Array_buffer<glm::vec3> triangle_vbo{triangle};
    gl::Vertex_array triangleVAO = make_vao(shader, triangle_vbo);

    Euler_perspective_camera camera;
    bool show_aabbs = true;
};


// public API

osc::Hittest_screen::Hittest_screen() :
    m_Impl{new Impl{}} {
}

osc::Hittest_screen::~Hittest_screen() noexcept = default;

void osc::Hittest_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Hittest_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Hittest_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().request_transition<Experiments_screen>();
    }
}

void osc::Hittest_screen::tick(float) {
    auto& camera = m_Impl->camera;
    auto& io = ImGui::GetIO();

    float speed = 10.0f;
    float sensitivity = 0.005f;

    if (io.KeysDown[SDL_SCANCODE_ESCAPE]) {
        App::cur().request_quit();
    }

    if (io.KeysDown[SDL_SCANCODE_W]) {
        camera.pos += speed * camera.front() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_S]) {
        camera.pos -= speed * camera.front() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_A]) {
        camera.pos -= speed * camera.right() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_D]) {
        camera.pos += speed * camera.right() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_SPACE]) {
        camera.pos += speed * camera.up() * io.DeltaTime;
    }

    if (io.KeyCtrl) {
        camera.pos -= speed * camera.up() * io.DeltaTime;
    }

    camera.yaw += sensitivity * io.MouseDelta.x;
    camera.pitch  -= sensitivity * io.MouseDelta.y;
    camera.pitch = std::clamp(camera.pitch, -fpi2 + 0.5f, fpi2 - 0.5f);


    // compute hits

    Line camera_line;
    camera_line.o = camera.pos;
    camera_line.d = camera.front();

    float closest_t = FLT_MAX;
    Scene_sphere* closest_ss = nullptr;

    for (Scene_sphere& ss : m_Impl->spheres) {
        ss.is_hovered = false;

        Sphere s;
        s.origin = ss.pos;
        s.radius = m_Impl->sphere_bound.radius;

        Ray_collision res = get_ray_collision_sphere(camera_line, s);
        if (res.hit && res.distance >= 0.0f && res.distance < closest_t) {
            closest_t = res.distance;
            closest_ss = &ss;
        }
    }

    if (closest_ss) {
        closest_ss->is_hovered = true;
    }

}

void osc::Hittest_screen::draw() {
    osc::ImGuiNewFrame();

    App& app = App::cur();
    Impl& impl = *m_Impl;
    Shader& shader = impl.shader;

    Line camera_line;
    camera_line.d = impl.camera.front();
    camera_line.o = impl.camera.pos;

    gl::Viewport(0, 0, App::cur().idims().x, App::cur().idims().y);
    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uView, impl.camera.view_matrix());
    gl::Uniform(shader.uProjection, impl.camera.projection_matrix(app.aspect_ratio()));

    // main scene render
    if (true) {
        gl::BindVertexArray(impl.sphere_vao);
        for (Scene_sphere const& s : impl.spheres) {
            glm::vec4 color = s.is_hovered ?
                glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} :
                glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};

            gl::Uniform(shader.uColor, color);
            gl::Uniform(shader.uModel, glm::translate(glm::mat4{1.0f}, s.pos));
            gl::DrawArrays(GL_TRIANGLES, 0, impl.sphere_vbo.sizei());
        }
        gl::BindVertexArray();
    }

    // AABBs
    if (impl.show_aabbs) {
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

        glm::vec3 half_widths = aabb_dims(impl.sphere_aabbs) / 2.0f;
        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, half_widths);

        gl::BindVertexArray(impl.cube_wireframe_vao);
        for (Scene_sphere const& s : impl.spheres) {
            glm::mat4 mover = glm::translate(glm::mat4{1.0f}, s.pos);
            gl::Uniform(shader.uModel, mover * scaler);
            gl::DrawArrays(GL_LINES, 0, impl.cube_wireframe_vbo.sizei());
        }
        gl::BindVertexArray();
    }

    // draw disc
    if (true) {
        Disc d;
        d.origin = {0.0f, 0.0f, 0.0f};
        d.normal = {0.0f, 1.0f, 0.0f};
        d.radius = {10.0f};

        Ray_collision res = get_ray_collision_disc(camera_line, d);

        glm::vec4 color = res.hit ?
            glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} :
            glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};

        Disc mesh_disc{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 1.0f};

        gl::Uniform(shader.uModel, disc_to_disc_xform(mesh_disc, d));
        gl::Uniform(shader.uColor, color);
        gl::BindVertexArray(impl.circle_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, impl.circle_vbo.sizei());
        gl::BindVertexArray();
    }

    // draw triangle
    if (true) {
        Ray_collision res = get_ray_collision_triangle(camera_line, impl.triangle.data());

        glm::vec4 color = res.hit ?
            glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} :
            glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};

        gl::Uniform(shader.uModel, gl::identity_val);
        gl::Uniform(shader.uColor, color);
        gl::BindVertexArray(impl.triangleVAO);
        gl::DrawArrays(GL_TRIANGLES, 0, impl.triangle_vbo.sizei());
        gl::BindVertexArray();
    }

    // crosshair
    if (true) {
        gl::Uniform(shader.uModel, gl::identity_val);
        gl::Uniform(shader.uView, gl::identity_val);
        gl::Uniform(shader.uProjection, gl::identity_val);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.crosshair_vao);
        gl::DrawArrays(GL_LINES, 0, impl.crosshair_vbo.sizei());
        gl::BindVertexArray();
    }

    osc::ImGuiRender();
}
