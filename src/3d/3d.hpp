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

namespace osc::todo {
    struct Untextured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;

        Untextured_vert() = default;
        constexpr Untextured_vert(glm::vec3 pos_, glm::vec3 normal_) noexcept : pos{pos_}, normal{normal_} {
        }
    };
    static_assert(std::is_trivial_v<Untextured_vert>);
    static_assert(sizeof(Untextured_vert) == 24);

    struct Textured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texcoord;

        Textured_vert() = default;
        constexpr Textured_vert(glm::vec3 pos_, glm::vec3 normal_, glm::vec2 texcoord_) noexcept :
            pos{pos_},
            normal{normal_},
            texcoord{texcoord_} {
        }
    };
    static_assert(std::is_trivial_v<Textured_vert>);
    static_assert(sizeof(Textured_vert) == 32);

    using elidx_t = GLushort;
    constexpr elidx_t elidx_max = std::numeric_limits<elidx_t>::max();

    template<typename TVert>
    struct CPU_mesh final {
        std::vector<TVert> verts;
        std::vector<elidx_t> indices;

        CPU_mesh() = default;

        CPU_mesh(std::vector<TVert> verts_) {
            verts = std::move(verts_);
            generate_trivial_indices();
        }

        CPU_mesh(TVert const* vs, size_t n) : CPU_mesh{std::vector(vs, vs + n)} {
        }

        template<size_t N>
        CPU_mesh(TVert const (&arr)[N]) : CPU_mesh(arr, N) {
        }

        template<typename Container>
        CPU_mesh(Container const& c) : CPU_mesh(c.data(), c.size()) {
        }

        void clear() {
            verts.clear();
            indices.clear();
        }

        void generate_trivial_indices() {
            indices.resize(verts.size());
            for (size_t i = 0; i < verts.size(); ++i) {
                indices[i] = static_cast<elidx_t>(i);
            }
        }
    };

    using Untextured_mesh = CPU_mesh<Untextured_vert>;
    using Textured_mesh = CPU_mesh<Textured_vert>;

    struct Rgba32 final {
        GLubyte r;
        GLubyte g;
        GLubyte b;
        GLubyte a;

        static constexpr Rgba32 from_normalized_floats(glm::vec4 const& v) noexcept {
            return Rgba32{
                static_cast<GLubyte>(255.0f * v.r),
                static_cast<GLubyte>(255.0f * v.g),
                static_cast<GLubyte>(255.0f * v.b),
                static_cast<GLubyte>(255.0f * v.a)
            };
        }

        Rgba32() = default;

        constexpr Rgba32(GLubyte _r, GLubyte _g, GLubyte _b, GLubyte _a) noexcept :
            r{_r},
            g{_g},
            b{_b},
            a{_a} {
        }
    };
    static_assert(std::is_trivial_v<Rgba32>);
    static_assert(sizeof(Rgba32) == 4);

    struct Rgb24 final {
        GLubyte r;
        GLubyte g;
        GLubyte b;

        Rgb24() = default;

        constexpr Rgb24(GLubyte _r, GLubyte _g, GLubyte _b) noexcept :
            r{_r}, g{_g}, b{_b} {
        }
    };
    static_assert(std::is_trivial_v<Rgb24>);
    static_assert(sizeof(Rgb24) == 3);

    struct Passthrough_data final {
        GLubyte b0;
        GLubyte b1;

        [[nodiscard]] static constexpr Passthrough_data from_u16(uint16_t v) noexcept {
            GLubyte b0 = v & 0xff;
            GLubyte b1 = (v >> 8) & 0xff;
            return Passthrough_data{b0, b1};
        }

        Passthrough_data() = default;

        [[nodiscard]] constexpr uint16_t to_u16() const noexcept {
            uint16_t rv = static_cast<uint16_t>(b0);
            rv |= static_cast<uint16_t>(b1) << 8;
            return rv;
        }

        [[nodiscard]] constexpr bool operator<(Passthrough_data const& other) noexcept {
            return this->to_u16() < other.to_u16();
        }
    };
    static_assert(std::is_trivial_v<Passthrough_data>);
    static_assert(sizeof(Passthrough_data) == 2);

    struct Instance_flags final {
        // layout (MSB to LSB)
        //
        // [0, 1) mode
        //     0 == GL_TRIANGLES (default)
        //     1 == GL_LINES
        // [1, 2) skip shading
        //     0 == do not skip shading (default)
        //     1 == skip shading
        // [2, 3) skip view projection
        //     0 == do not skip view projection (default)
        //     1 == skip view projection
        // [3, 8) padding, initialized to zeroes, so that the byte can be
        //        compared MSB to LSB with a single numeric comparison
        GLubyte data = 0x00;
        static constexpr GLubyte mode_mask = 0x80;
        static constexpr GLubyte skip_shading_mask = 0x40;
        static constexpr GLubyte skip_vp_mask = 0x20;

        constexpr Instance_flags() noexcept : data{0x00} {
        }

        [[nodiscard]] constexpr GLenum mode() const noexcept {
            return (data & mode_mask) ? GL_LINES : GL_TRIANGLES;
        }

        constexpr void set_mode_to_draw_lines() noexcept {
            data |= mode_mask;
        }

        [[nodiscard]] constexpr bool is_shaded() const noexcept {
            return !(data & skip_shading_mask);
        }

        constexpr void set_skip_shading() noexcept {
            data |= skip_shading_mask;
        }

        [[nodiscard]] constexpr bool skip_view_projection() const noexcept {
            return data & skip_vp_mask;
        }

        constexpr void set_skip_view_projection() noexcept {
            data |= skip_vp_mask;
        }

        constexpr bool operator==(Instance_flags other) const noexcept {
            return data == other.data;
        }

        constexpr bool operator!=(Instance_flags other) const noexcept {
            return data != other.data;
        }

        constexpr bool operator<(Instance_flags other) const noexcept {
            return data < other.data;
        }
    };

    // safe wrapper around a raw index type
    //
    // used so that the implementation can handle "plain" numbers with no RAII
    // overhead, but has some basic guarantees (initialization etc.)
    template<typename T>
    class Checked_idx {
    public:
        using value_type = T;
        static constexpr value_type invalid_value = -1;
        static constexpr value_type max_value = std::numeric_limits<T>::max();

        static_assert(std::is_integral_v<T>);
        static_assert(std::is_signed_v<T>);
        static_assert(invalid_value < 0);

    private:
        value_type id;

    public:
        static Checked_idx<T> from_index(size_t idx) {
            if (idx > max_value) {
                throw std::runtime_error{"Gpu_data_idx<T>::from_index: index too high: maybe too much data has been allocated on the GPU?"};
            }
            return Checked_idx{static_cast<T>(idx)};
        }

        constexpr Checked_idx() noexcept : id{invalid_value} {
        }

        constexpr Checked_idx(value_type _id) noexcept : id{_id} {
        }

        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return id >= 0;
        }

        constexpr bool operator==(Checked_idx<T> const& other) const noexcept {
            return id == other.id;
        }

        constexpr bool operator!=(Checked_idx<T> const& other) const noexcept {
            return id != other.id;
        }

        constexpr bool operator<(Checked_idx<T> const& other) const noexcept {
            return id < other.id;
        }

        [[nodiscard]] constexpr size_t to_index() const noexcept {
            OSC_ASSERT(is_valid());
            return static_cast<size_t>(id);
        }
    };

    using Mesh_idx = Checked_idx<short>;
    using Tex_idx = Checked_idx<short>;

    template<typename Mtx>
    static constexpr glm::mat3 normal_matrix(Mtx&& m) noexcept {
        glm::mat3 top_left{m};
        return glm::inverse(glm::transpose(top_left));
    }

    struct alignas(16) Mesh_instance final {
        // model-to-world xform
        glm::mat4x3 model_xform;

        // normal xform (normal matrix) for the above
        glm::mat3 normal_xform;

        // instance diffuse RGBA color (if no diffuse tex)
        Rgba32 rgba;

        // passthrough stuff (unshaded, used by rendering pipeline)
        union {
            struct {
                Passthrough_data passthrough;
                GLubyte rim_alpha;
            } data;
            Rgb24 color;
        } passthrough;
        static_assert(sizeof(passthrough) == 3);

        // rendering flags for this instance
        Instance_flags flags;

        // index of mesh in GPU_storage for this instance
        Mesh_idx meshidx;

        // (optional) index of texture in GPU_storage for this instanace
        Tex_idx texidx;
    };
    static_assert(sizeof(Mesh_instance) == 96);

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
        std::unordered_map<std::string, Mesh_idx> path_to_meshidx;

        // preallocated meshes
        Mesh_idx simbody_sphere_idx;
        Mesh_idx simbody_cylinder_idx;
        Mesh_idx simbody_cube_idx;
        Mesh_idx floor_quad_idx;
        Mesh_idx grid_25x25_idx;
        Mesh_idx yline_idx;
        Mesh_idx quad_idx;

        // preallocated textures
        Tex_idx chequer_idx;

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
        Passthrough_data hittest_result;

        Render_target(int w, int h, int samples);
        void reconfigure(int w, int h, int samples);
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
