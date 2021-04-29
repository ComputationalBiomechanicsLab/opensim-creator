#pragma once

#include "src/3d/gl.hpp"
#include "src/assertions.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>

#include <vector>
#include <limits>
#include <type_traits>
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <array>

namespace osc {
    struct Untextured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
    };

    struct Textured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texcoord;
    };

    // important: puts an upper limit on the number of verts that a single
    // mesh may contain
    using elidx_t = GLushort;

    template<typename TVert>
    struct CPU_mesh final {
        std::vector<TVert> verts;
        std::vector<elidx_t> indices;

        void clear() {
            verts.clear();
            indices.clear();
        }
    };

    template<typename TVert>
    void generate_1to1_indices_for_verts(CPU_mesh<TVert>& mesh) {
        size_t n = mesh.verts.size();
        mesh.indices.resize(n);
        for (size_t i = 0; i < n; ++i) {
            mesh.indices[i] = static_cast<elidx_t>(i);
        }
    }

    using Untextured_mesh = CPU_mesh<Untextured_vert>;
    using Textured_mesh = CPU_mesh<Textured_vert>;

    struct Rgba32 final {
        GLubyte r;
        GLubyte g;
        GLubyte b;
        GLubyte a;
    };

    struct Rgb24 final {
        GLubyte r;
        GLubyte g;
        GLubyte b;
    };

    // negative numbers are senteniels for "not in use" or "invalid"
    using meshidx_t = short;
    using texidx_t = short;

    // create a normal transform from a model transform matrix
    template<typename Mtx>
    static constexpr glm::mat3 normal_matrix(Mtx&& m) noexcept {
        glm::mat3 top_left{m};
        return glm::inverse(glm::transpose(top_left));
    }

    struct alignas(16) Mesh_instance final {
        glm::mat4x3 model_xform;
        glm::mat3 normal_xform;
        Rgba32 rgba;

        // blue: reserved for rim highlighting
        //
        // callers can encode whatever they want into the red and
        // green channels for runtime hit testing
        Rgb24 passthrough_color = {0x00, 0x00, 0x00};

        GLubyte flags = 0x00;
        static constexpr GLubyte draw_lines_mask = 0x80;
        static constexpr GLubyte skip_shading_mask = 0x40;
        static constexpr GLubyte skip_vp_mask = 0x20;

        texidx_t texidx = -1;
        meshidx_t meshidx = -1;
    };

    static GLenum draw_mode(Mesh_instance const& mi) {
        if (mi.flags & Mesh_instance::draw_lines_mask) {
            return GL_LINES;
        } else {
            return GL_TRIANGLES;
        }
    }

    // list of instances to draw in one renderer drawcall
    struct Drawlist final {
        // note: treat as private
        //
        // it might be that we switch this with memory mapping, etc.
        std::vector<Mesh_instance> _instances;

        [[nodiscard]] size_t size() const noexcept {
            return _instances.size();
        }

        template<typename... Args>
        Mesh_instance& emplace_back(Args... args) {
            return _instances.emplace_back(std::forward<Args>(args)...);
        }

        void clear() {
            _instances.clear();
        }

        template<typename Callback>
        void for_each(Callback f) {
            for (Mesh_instance& mi : _instances) {
                f(mi);
            }
        }
    };

    // optimize a drawlist
    //
    // (what is optimized is an internal detail: just assume that this function
    //  mutates the drawlist in some way to make a subsequent render call optimal)
    void optimize(Drawlist&) noexcept;

    // a mesh, stored on the GPU
    //
    // not in any particular format - depends on which CPU data was passed
    // into its constructor
    struct GPU_mesh final {
        gl::Array_buffer<GLubyte> verts;
        gl::Element_array_buffer<elidx_t> indices;
        gl::Array_buffer<Mesh_instance, GL_DYNAMIC_DRAW> instances;
        gl::Vertex_array main_vao;
        gl::Vertex_array normal_vao;
        bool is_textured : 1;

        GPU_mesh(Untextured_mesh const&);
        GPU_mesh(Textured_mesh const&);
    };

    struct Gouraud_mrt_shader;
    struct Normals_shader;
    struct Plain_texture_shader;
    struct Colormapped_plain_texture_shader;
    struct Edge_detection_shader;
    struct Skip_msxaa_blitter_shader;

    // storage for GPU data. Used by renderer to load relevant data at runtime
    // (e.g. shaders, programs, mesh data)
    struct GPU_storage final {
        std::unique_ptr<Gouraud_mrt_shader> shader_gouraud;
        std::unique_ptr<Normals_shader> shader_normals;
        std::unique_ptr<Plain_texture_shader> shader_pts;
        std::unique_ptr<Colormapped_plain_texture_shader> shader_cpts;
        std::unique_ptr<Edge_detection_shader> shader_eds;
        std::unique_ptr<Skip_msxaa_blitter_shader> shader_skip_msxaa;

        std::vector<GPU_mesh> meshes;
        std::vector<gl::Texture_2d> textures;
        std::unordered_map<std::string, meshidx_t> path_to_meshidx;

        // preallocated meshes
        meshidx_t simbody_sphere_idx;
        meshidx_t simbody_cylinder_idx;
        meshidx_t simbody_cube_idx;
        meshidx_t floor_quad_idx;
        meshidx_t grid_25x25_idx;
        meshidx_t yline_idx;
        meshidx_t quad_idx;

        // preallocated textures
        texidx_t chequer_idx;

        // debug quad
        gl::Array_buffer<Textured_vert> quad_vbo;

        // VAOs for debug quad
        gl::Vertex_array eds_quad_vao;
        gl::Vertex_array skip_msxaa_quad_vao;
        gl::Vertex_array pts_quad_vao;
        gl::Vertex_array cpts_quad_vao;

        GPU_storage();
        GPU_storage(GPU_storage const&) = delete;
        GPU_storage(GPU_storage&&) noexcept;
        GPU_storage& operator=(GPU_storage const&) = delete;
        GPU_storage& operator=(GPU_storage&&) noexcept;
        ~GPU_storage() noexcept;
    };

    // output target for a scene drawcall
    struct Render_target final {
        // dimensions of buffers
        int w;
        int h;

        // number of multisamples for multisampled buffers
        int samples;

        // raw scene output
        gl::Render_buffer scene_rgba;
        gl::Texture_2d_multisample scene_passthrough;
        gl::Render_buffer scene_depth24stencil8;
        gl::Frame_buffer scene_fbo;

        // passthrough resolution (intermediate data)
        gl::Texture_2d passthrough_nomsxaa;
        gl::Frame_buffer passthrough_fbo;
        std::array<gl::Pixel_pack_buffer<GLubyte, GL_STREAM_READ>, 2> passthrough_pbos;
        int passthrough_pbo_cur;

        // outputs
        gl::Texture_2d scene_tex_resolved;
        gl::Frame_buffer scene_fbo_resolved;
        gl::Texture_2d passthrough_tex_resolved;
        gl::Frame_buffer passthrough_fbo_resolved;
        Rgb24 hittest_result;

        Render_target(int w, int h, int samples);
        void reconfigure(int w, int h, int samples);

        [[nodiscard]] constexpr float aspect_ratio() const noexcept {
            return static_cast<float>(w) / static_cast<float>(h);
        }

        [[nodiscard]] gl::Texture_2d& main() noexcept {
            return scene_tex_resolved;
        }

        [[nodiscard]] constexpr glm::vec2 dimensions() const noexcept {
            return glm::vec2{static_cast<float>(w), static_cast<float>(h)};
        }
    };

    // flags for a scene drawcall
    using DrawcallFlags = int;
    enum DrawcallFlags_ {
        DrawcallFlags_None = 0 << 0,

        // draw meshes in wireframe mode
        DrawcallFlags_WireframeMode = 1 << 0,

        // draw mesh normals on top of render
        DrawcallFlags_ShowMeshNormals = 1 << 1,

        // draw selection rims
        DrawcallFlags_DrawRims = 1 << 2,

        // draw debug quads (development)
        RawRendererFlags_DrawDebugQuads = 1 << 3,

        // perform hit testing on Raw_mesh_instance passthrough data
        RawRendererFlags_PerformPassthroughHitTest = 1 << 4,

        // use optimized hit testing (which might arrive a frame late)
        RawRendererFlags_UseOptimizedButDelayed1FrameHitTest = 1 << 5,

        // draw the scene
        RawRendererFlags_DrawSceneGeometry = 1 << 6,

        // use instanced (optimized) rendering
        RawRendererFlags_UseInstancedRenderer = 1 << 7,

        RawRendererFlags_Default = DrawcallFlags_DrawRims | RawRendererFlags_DrawDebugQuads |
                                   RawRendererFlags_PerformPassthroughHitTest |
                                   RawRendererFlags_UseOptimizedButDelayed1FrameHitTest |
                                   RawRendererFlags_DrawSceneGeometry | RawRendererFlags_UseInstancedRenderer
    };

    // parameters for a scene drawcall
    struct Render_params final {
        glm::mat4 view_matrix;
        glm::mat4 projection_matrix;
        glm::vec3 view_pos;
        glm::vec3 light_pos;
        glm::vec3 light_rgb;
        glm::vec4 background_rgba;
        glm::vec4 rim_rgba;

        DrawcallFlags flags;
        int passthrough_hittest_x;
        int passthrough_hittest_y;
    };

    // draw a scene into the specified render target
    void draw_scene(GPU_storage&, Render_params const&, Drawlist const&, Render_target&);
}
