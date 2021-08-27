#include "mesh_hittest_with_bvh_screen.hpp"

#include "src/app.hpp"
#include "src/3d/bvh.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/3d/model.hpp"
#include "src/3d/shaders/solid_color_shader.hpp"
#include "src/screens/experimental/experiments_screen.hpp"
#include "src/simtk_bindings/stk_meshloader.hpp"

#include <imgui.h>

#include <vector>
#include <chrono>

using namespace osc;

static gl::Vertex_array make_vao(Solid_color_shader& shader, gl::Array_buffer<glm::vec3>& vbo, gl::Element_array_buffer<GLushort>& ebo) {
    gl::Vertex_array rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::BindBuffer(ebo);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(shader.aPos);
    gl::BindVertexArray();
    return rv;
}

// assumes vertex array is set. Only sets uModel and draws each frame
static void BVH_DrawRecursive(BVH const& bvh, Solid_color_shader& shader, int pos) {
    BVH_Node const& n = bvh.nodes[pos];

    glm::vec3 half_widths = aabb_dims(n.bounds) / 2.0f;
    glm::vec3 center = aabb_center(n.bounds);

    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, half_widths);
    glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
    glm::mat4 mmtx = mover * scaler;

    gl::Uniform(shader.uModel, mmtx);
    gl::DrawElements(GL_LINES, 24, GL_UNSIGNED_SHORT, nullptr);

    if (n.nlhs >= 0) {  // if it's an internal node
        BVH_DrawRecursive(bvh, shader, pos+1);
        BVH_DrawRecursive(bvh, shader, pos+n.nlhs+1);
    }
}

struct osc::Mesh_hittest_with_bvh_screen::Impl final {
    Solid_color_shader shader;

    NewMesh mesh = stk_load_mesh(App::resource("geometry/hat_ribs.vtp"));
    gl::Array_buffer<glm::vec3> mesh_vbo{mesh.verts};
    gl::Element_array_buffer<GLushort> mesh_ebo{mesh.indices};
    gl::Vertex_array mesh_vao = make_vao(shader, mesh_vbo, mesh_ebo);
    BVH mesh_bvh = BVH_CreateFromTriangles(mesh.verts.data(), mesh.verts.size());

    // triangle (debug)
    glm::vec3 tris[3];
    gl::Array_buffer<glm::vec3> triangle_vbo;
    gl::Element_array_buffer<GLushort> triangle_ebo = {0, 1, 2};
    gl::Vertex_array triangle_vao = make_vao(shader, triangle_vbo, triangle_ebo);

    // AABB wireframe
    NewMesh cube_wireframe = gen_cube_lines();
    gl::Array_buffer<glm::vec3> cube_wireframe_vbo{cube_wireframe.verts};
    gl::Element_array_buffer<GLushort> cube_wireframe_ebo{cube_wireframe.indices};
    gl::Vertex_array cube_vao = make_vao(shader, cube_wireframe_vbo, cube_wireframe_ebo);

    std::chrono::microseconds raycast_dur{0};
    Polar_perspective_camera camera;
    bool is_moused_over = false;
    bool use_BVH = true;
};

// public Impl

osc::Mesh_hittest_with_bvh_screen::Mesh_hittest_with_bvh_screen() :
    m_Impl{new Impl{}} {
}

osc::Mesh_hittest_with_bvh_screen::~Mesh_hittest_with_bvh_screen() noexcept = default;

void osc::Mesh_hittest_with_bvh_screen::on_mount() {
    osc::ImGuiInit();
    App::cur().disable_vsync();
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void osc::Mesh_hittest_with_bvh_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Mesh_hittest_with_bvh_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().request_transition<Experiments_screen>();
        return;
    }
}

void osc::Mesh_hittest_with_bvh_screen::tick(float) {
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
        Line camera_ray_worldspace = impl.camera.screenpos_to_world_ray(ImGui::GetMousePos(), App::cur().dims());
        // camera ray in worldspace == camera ray in model space because the model matrix is an identity matrix

        impl.is_moused_over = false;

        if (impl.use_BVH) {
            BVH_Collision res;
            if (BVH_get_closest_collision_triangle(impl.mesh_bvh, impl.mesh.verts.data(), impl.mesh.verts.size(), camera_ray_worldspace, &res)) {
                glm::vec3 const* v = impl.mesh.verts.data() + res.prim_id;
                impl.is_moused_over = true;
                impl.tris[0] = v[0];
                impl.tris[1] = v[1];
                impl.tris[2] = v[2];
                impl.triangle_vbo.assign(impl.tris, 3);
            }

        } else {
            auto const& verts = impl.mesh.verts;
            for (size_t i = 0; i < verts.size(); i += 3) {
                glm::vec3 tri[3] = {verts[i], verts[i+1], verts[i+2]};
                Ray_collision res = get_ray_collision_triangle(camera_ray_worldspace, tri);
                if (res.hit) {
                    impl.is_moused_over = true;

                    // draw triangle for hit
                    impl.tris[0] = tri[0];
                    impl.tris[1] = tri[1];
                    impl.tris[2] = tri[2];
                    impl.triangle_vbo.assign(impl.tris, 3);
                    break;
                }
            }
        }

    }
    auto raycast_end = std::chrono::high_resolution_clock::now();
    auto raycast_dt = raycast_end - raycast_start;
    impl.raycast_dur = std::chrono::duration_cast<std::chrono::microseconds>(raycast_dt);
}

void osc::Mesh_hittest_with_bvh_screen::draw() {
    auto dims = App::cur().idims();
    gl::Viewport(0, 0, dims.x, dims.y);

    osc::ImGuiNewFrame();

    Impl& impl = *m_Impl;
    Solid_color_shader& shader = impl.shader;

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // printout stats
    {
        ImGui::Begin("controls");
        ImGui::Text("raycast duration = %lld micros", impl.raycast_dur.count());
        ImGui::Checkbox("use BVH", &impl.use_BVH);
        ImGui::End();
    }

    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uModel, gl::identity_val);
    gl::Uniform(shader.uView, impl.camera.view_matrix());
    gl::Uniform(shader.uProjection, impl.camera.projection_matrix(App::cur().aspect_ratio()));
    gl::Uniform(shader.uColor, impl.is_moused_over ? glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} : glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
    if (true) {  // draw scene
        gl::BindVertexArray(impl.mesh_vao);
        gl::DrawElements(GL_TRIANGLES, impl.mesh_ebo.sizei(), gl::index_type(impl.mesh_ebo), nullptr);
        gl::BindVertexArray();
    }

    // draw hittest triangle debug
    if (impl.is_moused_over) {
        gl::Disable(GL_DEPTH_TEST);

        // draw triangle
        gl::Uniform(shader.uModel, gl::identity_val);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.triangle_vao);
        gl::DrawElements(GL_TRIANGLES, impl.triangle_ebo.sizei(), gl::index_type(impl.triangle_ebo), nullptr);
        gl::BindVertexArray();

        gl::Enable(GL_DEPTH_TEST);
    }

    // draw BVH
    if (impl.use_BVH && !impl.mesh_bvh.nodes.empty()) {
        // uModel is set by the recursive call

        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.cube_vao);
        BVH_DrawRecursive(impl.mesh_bvh, impl.shader, 0);
        gl::BindVertexArray();
    }

    osc::ImGuiRender();
}
