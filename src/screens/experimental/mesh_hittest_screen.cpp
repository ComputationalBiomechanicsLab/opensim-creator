#include "mesh_hittest_screen.hpp"

#include "src/app.hpp"
#include "src/log.hpp"
#include "src/3d/gl.hpp"
#include "src/simtk_bindings/simtk_bindings.hpp"

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

static gl::Vertex_array make_vao(Shader& shader, gl::Array_buffer<Untextured_vert>& vbo, gl::Element_array_buffer<GLushort>& ebo) {
    gl::Vertex_array rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::BindBuffer(ebo);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(Untextured_vert), offsetof(Untextured_vert, pos));
    gl::EnableVertexAttribArray(shader.aPos);
    gl::BindVertexArray();
    return rv;
}

struct osc::Mesh_hittesting::Impl final {
    Shader shader;

    Untextured_mesh mesh = []() {
        Untextured_mesh rv;
        stk_load_meshfile(App::resource("geometry/hat_ribs.vtp"), rv);
        return rv;
    }();

    gl::Array_buffer<Untextured_vert> mesh_vbo{mesh.verts};
    gl::Element_array_buffer<GLushort> mesh_ebo{mesh.indices};
    gl::Vertex_array mesh_vao = make_vao(shader, mesh_vbo, mesh_ebo);

    // sphere (debug)
    Untextured_mesh sphere = generate_uv_sphere<Untextured_mesh>();
    gl::Array_buffer<Untextured_vert> sphere_vbo{sphere.verts};
    gl::Element_array_buffer<GLushort> sphere_ebo{sphere.indices};
    gl::Vertex_array sphere_vao = make_vao(shader, sphere_vbo, sphere_ebo);

    // triangle (debug)
    Untextured_vert tris[3];
    gl::Array_buffer<Untextured_vert> triangle_vbo;
    gl::Element_array_buffer<GLushort> triangle_ebo = {0, 1, 2};
    gl::Vertex_array triangle_vao = make_vao(shader, triangle_vbo, triangle_ebo);

    // line (debug)
    gl::Array_buffer<Untextured_vert> line_vbo;
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
    osc::ImGuiOnEvent(e);
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
        glm::vec2 dims = App::cur().dims();
        float aspect_ratio = dims.x/dims.y;

        glm::mat4 proj_mtx = impl.camera.projection_matrix(aspect_ratio);
        glm::mat4 view_mtx = impl.camera.view_matrix();

        auto& io = ImGui::GetIO();

        // left-handed
        glm::vec2 mouse_pos_ndc = (2.0f * (glm::vec2{io.MousePos} / dims)) - 1.0f;
        mouse_pos_ndc.y = -mouse_pos_ndc.y;

        // location of mouse on NDC cube
        glm::vec4 line_origin_ndc = {mouse_pos_ndc.x, mouse_pos_ndc.y, -1.0f, 1.0f};

        // location of mouse in viewspace (worldspace, but everything moved with viewer @ 0,0,0)
        glm::vec4 line_origin_view = glm::inverse(proj_mtx) * line_origin_ndc;
        line_origin_view /= line_origin_view.w;  // perspective divide

        // location of mouse in worldspace
        glm::vec3 line_origin_world = glm::vec3{glm::inverse(view_mtx) * line_origin_view};

        // direction vector from camera to mouse location (i.e. the projection)
        glm::vec3 line_dir_world = glm::normalize(line_origin_world - impl.camera.pos());

        Line l;
        l.d = line_dir_world;
        l.o = line_origin_world;
        impl.ray = l;

        std::vector<glm::vec3> tris;
        tris.reserve(impl.mesh.verts.size());
        for (auto const& v : impl.mesh.verts) {
            tris.push_back(v.pos);
        }

        impl.is_moused_over = false;
        for (size_t i = 0; i < tris.size(); i += 3) {
            auto res = line_intersects_triangle(tris.data() + i, l);
            if (res.intersected) {
                impl.hitpos = l.o + res.t*l.d;
                impl.is_moused_over = true;
                impl.tris[0] = {tris[i], {}};
                impl.tris[1] = {tris[i+1], {}};
                impl.tris[2] = {tris[i+2], {}};
                impl.triangle_vbo.assign(impl.tris, 3);

                Untextured_vert lineverts[2] = {
                    {l.o, {}},
                    {l.o + 100.0f*l.d, {}}
                };
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
            ImGui::Text("p1 = (%.2f, %.2f, %.2f)", impl.tris[0].pos.x, impl.tris[0].pos.y, impl.tris[0].pos.z);
            ImGui::Text("p2 = (%.2f, %.2f, %.2f)", impl.tris[1].pos.x, impl.tris[1].pos.y, impl.tris[1].pos.z);
            ImGui::Text("p3 = (%.2f, %.2f, %.2f)", impl.tris[2].pos.x, impl.tris[2].pos.y, impl.tris[2].pos.z);

        }
        ImGui::End();
    }

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

