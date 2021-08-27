#include "mesh_hittest_screen.hpp"

#include "src/app.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/3d/model.hpp"
#include "src/screens/experimental/experiments_screen.hpp"
#include "src/simtk_bindings/stk_meshloader.hpp"

#include <glm/vec3.hpp>
#include <imgui.h>

#include <chrono>

using namespace osc;

static char const g_VertexShader[] = R"(
    #version 330 core

    uniform mat4 uProjMat;
    uniform mat4 uViewMat;
    uniform mat4 uModelMat;

    layout (location = 0) in vec3 aPos;

    void main() {
        gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);
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
    struct Shader final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::Vertex_shader>(g_VertexShader),
            gl::CompileFromSource<gl::Fragment_shader>(g_FragmentShader));

        gl::Attribute_vec3 aPos{0};

        gl::Uniform_mat4 uModel = gl::GetUniformLocation(prog, "uModelMat");
        gl::Uniform_mat4 uView = gl::GetUniformLocation(prog, "uViewMat");
        gl::Uniform_mat4 uProjection = gl::GetUniformLocation(prog, "uProjMat");
        gl::Uniform_vec4 uColor = gl::GetUniformLocation(prog, "uColor");
    };
}

static gl::Vertex_array make_vao(Shader& shader, gl::Array_buffer<glm::vec3>& vbo, gl::Element_array_buffer<GLushort>& ebo) {
    gl::Vertex_array rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::BindBuffer(ebo);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(shader.aPos);
    gl::BindVertexArray();
    return rv;
}

struct osc::Mesh_hittesting::Impl final {
    Shader shader;

    NewMesh mesh = stk_load_mesh(App::resource("geometry/hat_ribs.vtp"));
    gl::Array_buffer<glm::vec3> mesh_vbo{mesh.verts};
    gl::Element_array_buffer<GLushort> mesh_ebo{mesh.indices};
    gl::Vertex_array mesh_vao = make_vao(shader, mesh_vbo, mesh_ebo);

    // sphere (debug)
    NewMesh sphere = gen_untextured_uv_sphere(12, 12);
    gl::Array_buffer<glm::vec3> sphere_vbo{sphere.verts};
    gl::Element_array_buffer<GLushort> sphere_ebo{sphere.indices};
    gl::Vertex_array sphere_vao = make_vao(shader, sphere_vbo, sphere_ebo);

    // triangle (debug)
    glm::vec3 tris[3];
    gl::Array_buffer<glm::vec3> triangle_vbo;
    gl::Element_array_buffer<GLushort> triangle_ebo = {0, 1, 2};
    gl::Vertex_array triangle_vao = make_vao(shader, triangle_vbo, triangle_ebo);

    // line (debug)
    gl::Array_buffer<glm::vec3> line_vbo;
    gl::Element_array_buffer<GLushort> line_ebo = {0, 1};
    gl::Vertex_array line_vao = make_vao(shader, line_vbo, line_ebo);

    std::chrono::microseconds raycast_dur{0};
    Polar_perspective_camera camera;
    bool is_moused_over = false;
    glm::vec3 hitpos = {0.0f, 0.0f, 0.0f};

    Line ray;
};


// public Impl.

osc::Mesh_hittesting::Mesh_hittesting() :
    m_Impl{new Impl{}} {
}

osc::Mesh_hittesting::~Mesh_hittesting() noexcept = default;

void osc::Mesh_hittesting::on_mount() {
    osc::ImGuiInit();
    App::cur().disable_vsync();
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void osc::Mesh_hittesting::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Mesh_hittesting::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().request_transition<Experiments_screen>();
        return;
    }
}

void osc::Mesh_hittesting::tick(float) {
    Impl& impl = *m_Impl;

    impl.camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/10.0f;

    // handle panning/zooming/dragging with middle mouse
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {

        // in pixels, e.g. [800, 600]
        glm::vec2 screendims = App::cur().dims();

        // in pixels, e.g. [-80, 30]
        glm::vec2 mouse_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle, 0.0f);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);

        // as a screensize-independent ratio, e.g. [-0.1, 0.05]
        glm::vec2 relative_delta = mouse_delta / screendims;

        if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
            // shift + middle-mouse performs a pan
            float aspect_ratio = screendims.x / screendims.y;
            impl.camera.do_pan(aspect_ratio, relative_delta);
        } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
            // shift + middle-mouse performs a zoom
            impl.camera.radius *= 1.0f + relative_delta.y;
        } else {
            // just middle-mouse performs a mouse drag
            impl.camera.do_drag(relative_delta);
        }
    }

    // handle hittest
    auto raycast_start = std::chrono::high_resolution_clock::now();
    {

        impl.ray = impl.camera.screenpos_to_world_ray(ImGui::GetIO().MousePos, App::cur().dims());

        impl.is_moused_over = false;
        std::vector<glm::vec3> const& tris = impl.mesh.verts;
        for (size_t i = 0; i < tris.size(); i += 3) {
            Ray_collision res = get_ray_collision_triangle(impl.ray, tris.data() + i);
            if (res.hit) {
                impl.hitpos = impl.ray.o + res.distance*impl.ray.d;
                impl.is_moused_over = true;
                impl.tris[0] = tris[i];
                impl.tris[1] = tris[i+1];
                impl.tris[2] = tris[i+2];
                impl.triangle_vbo.assign(impl.tris, 3);

                glm::vec3 lineverts[2] = {impl.ray.o, impl.ray.o + 100.0f*impl.ray.d};
                impl.line_vbo.assign(lineverts, 2);

                break;
            }
        }
    }
    auto raycast_end = std::chrono::high_resolution_clock::now();
    auto raycast_dt = raycast_end - raycast_start;
    impl.raycast_dur = std::chrono::duration_cast<std::chrono::microseconds>(raycast_dt);
}

void osc::Mesh_hittesting::draw() {
    osc::ImGuiNewFrame();

    Impl& impl = *m_Impl;
    Shader& shader = impl.shader;

    // printout stats
    {
        ImGui::Begin("controls");
        ImGui::Text("%lld microseconds", impl.raycast_dur.count());
        auto r = impl.ray;
        ImGui::Text("camerapos = (%.2f, %.2f, %.2f)", impl.camera.pos().x, impl.camera.pos().y, impl.camera.pos().z);
        ImGui::Text("origin = (%.2f, %.2f, %.2f), dir = (%.2f, %.2f, %.2f)", r.o.x, r.o.y, r.o.z, r.d.x, r.d.y, r.d.z);
        if (impl.is_moused_over) {
            ImGui::Text("hit = (%.2f, %.2f, %.2f)", impl.hitpos.x, impl.hitpos.y, impl.hitpos.z);
            ImGui::Text("p1 = (%.2f, %.2f, %.2f)", impl.tris[0].x, impl.tris[0].y, impl.tris[0].z);
            ImGui::Text("p2 = (%.2f, %.2f, %.2f)", impl.tris[1].x, impl.tris[1].y, impl.tris[1].z);
            ImGui::Text("p3 = (%.2f, %.2f, %.2f)", impl.tris[2].x, impl.tris[2].y, impl.tris[2].z);

        }
        ImGui::End();
    }

    gl::Viewport(0, 0, App::cur().idims().x, App::cur().idims().y);
    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uModel, gl::identity_val);
    gl::Uniform(shader.uView, impl.camera.view_matrix());
    gl::Uniform(shader.uProjection, impl.camera.projection_matrix(App::cur().aspect_ratio()));
    gl::Uniform(shader.uColor, impl.is_moused_over ? glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} : glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
    if (true) {
        gl::BindVertexArray(impl.mesh_vao);
        gl::DrawElements(GL_TRIANGLES, impl.mesh_ebo.sizei(), gl::index_type(impl.mesh_ebo), nullptr);
        gl::BindVertexArray();
    }


    if (impl.is_moused_over) {

        gl::Disable(GL_DEPTH_TEST);

        // draw sphere
        gl::Uniform(shader.uModel, glm::translate(glm::mat4{1.0f}, impl.hitpos) * glm::scale(glm::mat4{1.0f}, {0.01f, 0.01f, 0.01f}));
        gl::Uniform(shader.uColor, {1.0f, 1.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.sphere_vao);
        gl::DrawElements(GL_TRIANGLES, impl.sphere_ebo.sizei(), gl::index_type(impl.sphere_ebo), nullptr);
        gl::BindVertexArray();

        // draw triangle
        gl::Uniform(shader.uModel, gl::identity_val);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.triangle_vao);
        gl::DrawElements(GL_TRIANGLES, impl.triangle_ebo.sizei(), gl::index_type(impl.triangle_ebo), nullptr);
        gl::BindVertexArray();

        // draw line
        gl::Uniform(shader.uModel, gl::identity_val);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.line_vao);
        gl::DrawElements(GL_LINES, impl.line_ebo.sizei(), gl::index_type(impl.line_ebo), nullptr);
        gl::BindVertexArray();

        gl::Enable(GL_DEPTH_TEST);
    }

    osc::ImGuiRender();
}

