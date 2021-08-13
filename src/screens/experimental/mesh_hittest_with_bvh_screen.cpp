#include "mesh_hittest_with_bvh_screen.hpp"

#include "src/app.hpp"
#include "src/3d/3d.hpp"
#include "src/3d/gl.hpp"
#include "src/simtk_bindings/simtk_bindings.hpp"

#include <imgui.h>

#include <vector>
#include <chrono>

using namespace osc;

namespace {

    // a node in the tree
    struct BVH_Node final {
        AABB bounds;  // union of all AABBs below/including this one
        int lhs;  // index of left-hand node, -1 if leaf
        int rhs;  // index of right-hand node, -1 if leaf
        int firstPrimOffset;  // offset into primitives array
        int nPrims;  // number of primitives this node represents
    };

    // primitive info - usually, derived from the source collection
    struct BVH_Prim final {
        int id;  // ID into source collection (e.g. a mesh)
        AABB bounds;  // AABB of the prim (this is all the BVH cares about)
    };

    struct BVH final {
        std::vector<BVH_Node> nodes;  // nodes of the tree
        std::vector<BVH_Prim> prims;  // related primitive info
    };

    void BVH_Recurse(BVH& bvh, int begin, int n) {
        int end = begin + n;

        // if recursion bottoms out, create leaf node
        if (n == 1) {
            BVH_Node& leaf = bvh.nodes.emplace_back();
            leaf.bounds = bvh.prims[begin].bounds;
            leaf.lhs = -1;
            leaf.rhs = -1;
            leaf.firstPrimOffset = begin;
            leaf.nPrims = 1;
            return;
        }

        // else: compute internal node
        OSC_ASSERT(n > 1);

        // compute bounding box of remaining prims
        AABB aabb{{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
        for (int i = begin; i < end; ++i) {
            aabb = aabb_union(aabb, bvh.prims[i].bounds);
        }

        // edge-case: if it's empty, return a leaf node
        if (aabb_is_empty(aabb)) {
            BVH_Node& leaf = bvh.nodes.emplace_back();
            leaf.bounds = aabb;
            leaf.lhs = -1;
            leaf.rhs = -1;
            leaf.firstPrimOffset = begin;
            leaf.nPrims = n;
            return;
        }

        // compute slicing position along the longest dimension
        auto dim_idx = aabb_longest_dim_idx(aabb);
        float midpoint_x2 = aabb.min[dim_idx] + aabb.max[dim_idx];

        // returns true if a given primitive is below the midpoint along the dim
        auto is_below_midpoint = [dim_idx, midpoint_x2](BVH_Prim const& p) {
            float prim_midpoint_x2 = p.bounds.min[dim_idx] + p.bounds.max[dim_idx];
            return prim_midpoint_x2 <= midpoint_x2;
        };

        // partition prims into above/below the midpoint
        auto it = std::partition(bvh.prims.begin() + begin, bvh.prims.begin() + end, is_below_midpoint);
        int mid = static_cast<int>(std::distance(bvh.prims.begin(), it));

        // edge-case: failed to spacially partition: just naievely partition
        if (!(begin < mid && mid < end)) {
            mid = begin + n/2;
        }

        OSC_ASSERT(begin < mid && mid < end);

        // allocate internal node
        int internal_loc = static_cast<int>(bvh.nodes.size());  // careful: reallocations
        bvh.nodes.emplace_back();
        bvh.nodes[internal_loc].firstPrimOffset = -1;
        bvh.nodes[internal_loc].nPrims = 0;

        // build left node
        bvh.nodes[internal_loc].lhs = static_cast<int>(bvh.nodes.size());
        BVH_Recurse(bvh, begin, mid-begin);

        // build right node
        bvh.nodes[internal_loc].rhs = static_cast<int>(bvh.nodes.size());
        BVH_Recurse(bvh, mid, end - mid);

        // compute internal node's bounds from the left+right side
        bvh.nodes[internal_loc].bounds = aabb_union(
            bvh.nodes[bvh.nodes[internal_loc].lhs].bounds,
            bvh.nodes[bvh.nodes[internal_loc].rhs].bounds);
    }

    void BVH_Build(BVH& bvh, std::vector<Untextured_vert> const& triangles) {
        OSC_ASSERT(triangles.size()/3 <= std::numeric_limits<int>::max());

        // clear out any old data
        bvh.nodes.clear();
        bvh.prims.clear();

        // build up the prim list for each triangle
        for (size_t i = 0; i < triangles.size(); i += 3) {
            glm::vec3 verts[3] = {
                triangles[i+0].pos,
                triangles[i+1].pos,
                triangles[i+2].pos,
            };
            BVH_Prim& prim = bvh.prims.emplace_back();
            prim.bounds = aabb_from_triangle(verts);
            prim.id = static_cast<int>(i);
        }

        // recursively build the tree
        BVH_Recurse(bvh, 0, static_cast<int>(bvh.prims.size()));
    }

    BVH BVH_Create(std::vector<Untextured_vert> const& triangles) {
        BVH rv;
        BVH_Build(rv, triangles);
        return rv;
    }

    Untextured_vert const* BVH_check_collision_recursive(BVH const& bvh, std::vector<Untextured_vert> const& tris, Line const& ray, int nodeidx) {
        BVH_Node const& n = bvh.nodes[nodeidx];

        auto res = line_intersects_AABB(n.bounds, ray);

        if (!res.intersected) {
            return nullptr;  // no intersection at all
        }

        if (n.lhs == -1 && n.rhs == -1) {
            // leaf node: check intersection with primitives

            for (int i = n.firstPrimOffset, end = n.firstPrimOffset + n.nPrims; i < end; ++i) {
                BVH_Prim const& p = bvh.prims[i];

                Untextured_vert const* first = tris.data() + p.id;
                glm::vec3 tri[3] = {first[0].pos, first[1].pos, first[2].pos};

                auto raytri = line_intersects_triangle(tri, ray);
                if (raytri.intersected) {
                    return first;
                }
            }

            return nullptr;
        } else {
            // internal node: check intersection with direct children

            // lhs test
            Untextured_vert const* lhs = BVH_check_collision_recursive(bvh, tris, ray, n.lhs);
            if (lhs) {
                return lhs;
            }

            // rhs test
            return BVH_check_collision_recursive(bvh, tris, ray, n.rhs);
        }
    }

    // returns pointer to first triangle that BVH detected intersects with ray
    //
    // note: no spatial ordering, just returns whatever was found first
    Untextured_vert const* BVH_check_collision(BVH const& bvh, std::vector<Untextured_vert> const& tris, Line const& ray) {
        OSC_ASSERT(bvh.prims.size() == tris.size()/3);

        if (bvh.nodes.empty()) {
            return nullptr;
        }

        if (bvh.prims.empty()) {
            return nullptr;
        }

        if (tris.empty()) {
            return nullptr;
        }

        return BVH_check_collision_recursive(bvh, tris, ray, 0);
    }
}

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

struct osc::Mesh_hittest_with_bvh_screen::Impl final {
    Shader shader;

    Untextured_mesh mesh = []() {
        Untextured_mesh rv;
        stk_load_meshfile(App::resource("geometry/hat_ribs.vtp"), rv);
        return rv;
    }();
    gl::Array_buffer<Untextured_vert> mesh_vbo{mesh.verts};
    gl::Element_array_buffer<GLushort> mesh_ebo{mesh.indices};
    gl::Vertex_array mesh_vao = make_vao(shader, mesh_vbo, mesh_ebo);
    BVH mesh_bvh = BVH_Create(mesh.verts);

    // triangle (debug)
    Untextured_vert tris[3];
    gl::Array_buffer<Untextured_vert> triangle_vbo;
    gl::Element_array_buffer<GLushort> triangle_ebo = {0, 1, 2};
    gl::Vertex_array triangle_vao = make_vao(shader, triangle_vbo, triangle_ebo);

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
    osc::ImGuiOnEvent(e);
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

        impl.is_moused_over = false;

        if (impl.use_BVH) {
            Untextured_vert const* v = BVH_check_collision(impl.mesh_bvh, impl.mesh.verts, l);
            if (v) {
                impl.is_moused_over = true;
                impl.tris[0] = {v[0].pos, {}};
                impl.tris[1] = {v[1].pos, {}};
                impl.tris[2] = {v[2].pos, {}};
                impl.triangle_vbo.assign(impl.tris, 3);
            }

        } else {
            auto const& verts = impl.mesh.verts;
            for (size_t i = 0; i < verts.size(); i += 3) {
                glm::vec3 tri[3] = {verts[i].pos, verts[i+1].pos, verts[i+2].pos};
                auto res = line_intersects_triangle(tri, l);
                if (res.intersected) {
                    impl.is_moused_over = true;

                    // draw triangle for hit
                    impl.tris[0] = {tri[0], {}};
                    impl.tris[1] = {tri[1], {}};
                    impl.tris[2] = {tri[2], {}};
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
    Shader& shader = impl.shader;

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

    // draw hittest debug
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

    osc::ImGuiRender();
}
