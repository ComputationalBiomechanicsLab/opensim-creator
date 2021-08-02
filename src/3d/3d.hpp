#pragma once

#include "src/3d/gl.hpp"

#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <limits>
#include <type_traits>
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <array>
#include <iosfwd>

// 3d: low-level 3D rendering primitives based on OpenGL
//
// these are the low-level datastructures/functions used for rendering
// 3D elements in OSC. The renderer is not dependent on SimTK/OpenSim at
// all and has a very low-level view of view of things (verts, drawlists)
namespace osc {
    // helpers for printing glm types
    //
    // handy for debugging 3D maths
    std::ostream& operator<<(std::ostream& o, glm::vec3 const& v);

    // returns true if the provided vectors are at the same location
    [[nodiscard]] bool is_colocated(glm::vec3 const&, glm::vec3 const&) noexcept;

    // a basic position in space + normal (for shading calculations)
    struct Untextured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
    };

    // as above, but with texture coordinate data also (for texture mapping)
    struct Textured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texcoord;
    };

    // datatype for vertex indices
    //
    // meshes are rendered by supplying vertex data (e.g. xy positions, texcoords) and
    // a list of indices to the vertex data. This is so that vertices can be shared
    // between geometry primitives (e.g. corners of a triangle can be shared).
    //
    // note: this puts an upper limit on the number of elements a single mesh can contain
    using elidx_t = GLushort;

    // raw mesh data, held in main-memory and accessible to the CPU
    template<typename TVert>
    struct CPU_mesh {

        // raw vert data
        std::vector<TVert> verts;

        // indices into the above
        std::vector<elidx_t> indices;

        void clear() {
            verts.clear();
            indices.clear();
        }
    };

    // specializations of CPU_mesh
    struct Untextured_mesh : public CPU_mesh<Untextured_vert> {};
    struct Textured_mesh : public CPU_mesh<Textured_vert> {};

    // generate 1:1 indices for each vert in a CPU_mesh
    //
    // effectively, generate the indices [0, 1, ..., size-1]
    template<typename TVert>
    void generate_1to1_indices_for_verts(CPU_mesh<TVert>& mesh) {

        // edge-case: ensure the caller isn't trying to used oversized indices
        if (mesh.verts.size() > std::numeric_limits<elidx_t>::max()) {
            throw std::runtime_error{"cannot generate indices for a mesh: the mesh has too many vertices: if you need to support this many vertices then contact the developers"};
        }

        size_t n = mesh.verts.size();
        mesh.indices.resize(n);
        for (size_t i = 0; i < n; ++i) {
            mesh.indices[i] = static_cast<elidx_t>(i);
        }
    }

    inline constexpr glm::vec3 lower_bound(glm::vec3 const& a, glm::vec3 const& b) noexcept {
        return glm::vec3{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
    }

    inline constexpr glm::vec3 upper_bound(glm::vec3 const& a, glm::vec3 const& b) noexcept {
        return glm::vec3{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
    }

    // axis-aligned bounding box
    struct AABB final {
        glm::vec3 min;
        glm::vec3 max;
    };

    // returns the centerpoint of an AABB
    inline constexpr glm::vec3 aabb_center(AABB const& a) noexcept {
        return (a.min + a.max)/2.0f;
    }

    // returns the smallest AABB that spans all AABBs in the range [it, end)
    //
    // `AABBGetter` should be a callable with signature: `AABB const&(*)(decltype(*it))`
    template<typename ForwardIterator, typename AABBGetter>
    inline constexpr AABB aabb_union(ForwardIterator it, ForwardIterator end, AABBGetter getter) noexcept {
        if (it == end) {
            return AABB{};  // instead of throwing, which might be better behavior...
        }

        AABB rv = getter(*it++);
        while (it != end) {
            rv = aabb_union(rv, getter(*it++));
        }
        return rv;
    }

    // returns the smallest AABB that spans both of the provided AABBs
    inline constexpr AABB aabb_union(AABB const& a, AABB const& b) noexcept {
        return AABB{lower_bound(a.min, b.min), upper_bound(a.max, b.max)};
    }

    // returns the dimensions of an AABB
    inline constexpr glm::vec3 aabb_dims(AABB const& a) noexcept {
        return a.max - a.min;
    }

    // returns the eight corner points of the cuboid representation of the AABB
    inline std::array<glm::vec3, 8> aabb_verts(AABB const& aabb) noexcept {

        glm::vec3 d = aabb_dims(aabb);

        // effectively, span each dimension from each AABB corner
        return std::array<glm::vec3, 8>{{
            aabb.min,
            aabb.min + d.x,
            aabb.min + d.y,
            aabb.min + d.z,
            aabb.max,
            aabb.max - d.x,
            aabb.max - d.y,
            aabb.max - d.z,
        }};
    }

    // apply a transformation matrix to the AABB, returning a new AABB that
    // spans the existing AABB's cuboid
    //
    // note: repeatably doing this can keep growing the AABB, even if the
    // underlying data wouldn't logically grow. Think about what would happen
    // if you rotated an AABB 45 degrees in one axis, successively returning
    // a new AABB that had to expand to accomodate the diagonal growth.
    inline AABB operator*(glm::mat4 const& m, AABB const& aabb) noexcept {
        auto verts = aabb_verts(aabb);
        for (auto& vert : verts) {
            vert = m * glm::vec4{vert, 1.0f};
        }

        AABB rv{verts[0], verts[0]};
        for (int i = 1; i < verts.size(); ++i) {
            rv.min = lower_bound(rv.min, verts[i]);
            rv.max = upper_bound(rv.max, verts[i]);
        }
        return rv;
    }

    // for debug printing
    std::ostream& operator<<(std::ostream& o, AABB const& aabb);

    // computes an AABB from a mesh
    [[nodiscard]] AABB aabb_from_mesh(Untextured_mesh const&) noexcept;

    // sphere (principally, for bounding-sphere calcs)
    struct Sphere final {
        glm::vec3 origin;
        float radius;
    };

    [[nodiscard]] Sphere bounding_sphere_from_mesh(Untextured_mesh const&) noexcept;

    [[nodiscard]] inline constexpr float longest_dimension(glm::vec3 const& v) noexcept {
        return std::max(std::max(v.x, v.y), v.z);
    }

    [[nodiscard]] inline constexpr GLubyte f32_to_u8_color(float v) noexcept {
        return static_cast<GLubyte>(255.0f * v);
    }

    [[nodiscard]] inline constexpr float u8_to_f32_color(GLubyte b) noexcept {
        return static_cast<float>(b) / 255.0f;
    }

    [[nodiscard]] inline constexpr GLubyte f64_to_u8_color(double v) noexcept {
        return static_cast<GLubyte>(255.0 * v);
    }

    // 4 color channels (RGBA), 8 bits per channel
    struct Rgba32 final {
        GLubyte r;
        GLubyte g;
        GLubyte b;
        GLubyte a;

        [[nodiscard]] static constexpr Rgba32 from_vec4(glm::vec4 const& v) noexcept {
            Rgba32 rv{};
            rv.r = f32_to_u8_color(v.r);
            rv.g = f32_to_u8_color(v.g);
            rv.b = f32_to_u8_color(v.b);
            rv.a = f32_to_u8_color(v.a);
            return rv;
        }

        [[nodiscard]] static constexpr Rgba32 from_d4(double r, double g, double b, double a) noexcept {
            Rgba32 rv{};
            rv.r = f64_to_u8_color(r);
            rv.g = f64_to_u8_color(g);
            rv.b = f64_to_u8_color(b);
            rv.a = f64_to_u8_color(a);
            return rv;
        }

        [[nodiscard]] static constexpr Rgba32 from_f4(float r, float g, float b, float a) noexcept {
            Rgba32 rv{};
            rv.r = f32_to_u8_color(r);
            rv.g = f32_to_u8_color(g);
            rv.b = f32_to_u8_color(b);
            rv.a = f32_to_u8_color(a);
            return rv;
        }

        [[nodiscard]] static constexpr Rgba32 from_u32(uint32_t v) noexcept {
            Rgba32 rv{};
            rv.r = static_cast<GLubyte>((v >> 24) & 0xff);
            rv.g = static_cast<GLubyte>((v >> 16) & 0xff);
            rv.b = static_cast<GLubyte>((v >> 8) & 0xff);
            rv.a = static_cast<GLubyte>(v & 0xff);
            return rv;
        }

        Rgba32() = default;

        constexpr Rgba32(GLubyte r_, GLubyte g_, GLubyte b_, GLubyte a_) noexcept :
            r{r_},
            g{g_},
            b{b_},
            a{a_} {
        }

        [[nodiscard]] constexpr uint32_t to_u32() const noexcept {
            uint32_t rv = static_cast<uint32_t>(r);
            rv |= static_cast<uint32_t>(g) << 8;
            rv |= static_cast<uint32_t>(b) << 16;
            rv |= static_cast<uint32_t>(a) << 24;
            return rv;
        }

        [[nodiscard]] constexpr glm::vec4 to_vec4() const noexcept {
            glm::vec4 rv{};
            rv.r = u8_to_f32_color(r);
            rv.g = u8_to_f32_color(g);
            rv.b = u8_to_f32_color(b);
            rv.a = u8_to_f32_color(a);
            return rv;
        }
    };

    // 3 color channels (RGB), 8 bits per channel
    struct Rgb24 final {
        GLubyte r;
        GLubyte g;
        GLubyte b;

        [[nodiscard]] static constexpr Rgb24 from_vec3(glm::vec3 const& v) noexcept {
            Rgb24 rv{};
            rv.r = static_cast<GLubyte>(255.0f * v.r);
            rv.g = static_cast<GLubyte>(255.0f * v.g);
            rv.b = static_cast<GLubyte>(255.0f * v.b);
            return rv;
        }

        Rgb24() = default;

        constexpr Rgb24(GLubyte r_, GLubyte g_, GLubyte b_) noexcept :
            r{r_},
            g{g_},
            b{b_} {
        }
    };

    // flags for a single mesh instance
    //
    // e.g. what draw mode it should be rendered with, what shading it should/shouln't have, etc.
    class Instance_flags final {
        GLubyte flags = 0x00;
        static constexpr GLubyte draw_lines_mask = 0x80;
        static constexpr GLubyte skip_shading_mask = 0x40;
        static constexpr GLubyte skip_vp_mask = 0x20;

    public:
        [[nodiscard]] constexpr GLenum mode() const noexcept {
            return flags & draw_lines_mask ? GL_LINES : GL_TRIANGLES;
        }

        void set_draw_lines() noexcept {
            flags |= draw_lines_mask;
        }

        [[nodiscard]] constexpr bool skip_shading() const noexcept {
            return flags & skip_shading_mask;
        }

        void set_skip_shading() noexcept {
            flags |= skip_shading_mask;
        }

        [[nodiscard]] constexpr bool skip_vp() const noexcept {
            return flags & skip_vp_mask;
        }

        void set_skip_vp() noexcept {
            flags |= skip_vp_mask;
        }

        [[nodiscard]] constexpr bool operator<(Instance_flags other) const noexcept {
            return flags < other.flags;
        }

        [[nodiscard]] constexpr bool operator==(Instance_flags other) const noexcept {
            return flags == other.flags;
        }

        [[nodiscard]] constexpr bool operator!=(Instance_flags other) const noexcept {
            return flags != other.flags;
        }
    };

    // a runtime-checked "index" value with support for senteniel values (negative values)
    // that indicate "invalid index"
    //
    // the utility of this is for using undersized index types (e.g. `short`, 32-bit ints,
    // etc.). The perf hit from runtime-checking is typically outweighed by the reduction
    // of memory use, resulting in fewer cache misses, etc..
    //
    // the `Derived` template param is to implement the "curously recurring template
    // pattern" (CRTP) - google it
    template<typename T, typename Derived>
    class Checked_index {
    public:
        using value_type = T;
        static constexpr value_type invalid_value = -1;
        static_assert(std::is_signed_v<value_type>);
        static_assert(std::is_integral_v<value_type>);

    private:
        value_type v;

    public:
        // curously-recurring template pattern
        [[nodiscard]] static constexpr Derived from_index(size_t i) {
            if (i > std::numeric_limits<value_type>::max()) {
                throw std::runtime_error{"OSMV error: tried to create a Safe_index with a value that is too high for the underlying value type"};
            }
            return Derived{static_cast<T>(i)};
        }

        constexpr Checked_index() noexcept : v{invalid_value} {
        }

        explicit constexpr Checked_index(value_type v_) noexcept : v{v_} {
        }

        [[nodiscard]] constexpr value_type get() const noexcept {
            return v;
        }

        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return v >= 0;
        }

        [[nodiscard]] constexpr size_t as_index() const {
            if (!is_valid()) {
                throw std::runtime_error{"OSMV error: tried to convert a Safe_index with an invalid value into an index"};
            }
            return static_cast<size_t>(v);
        }

        [[nodiscard]] constexpr bool operator<(Checked_index<T, Derived> other) const noexcept {
            return v < other.v;
        }

        [[nodiscard]] constexpr bool operator==(Checked_index<T, Derived> other) const noexcept {
            return v == other.v;
        }

        [[nodiscard]] constexpr bool operator!=(Checked_index<T, Derived> other) const noexcept {
            return v != other.v;
        }
    };

    class Meshidx : public Checked_index<short, Meshidx> {
        using Checked_index<short, Meshidx>::Checked_index;
    };

    class Texidx : public Checked_index<short, Texidx> {
        using Checked_index<short, Texidx>::Checked_index;
    };

    // generate a chequered floor texture
    //
    // this is typically used as a default scene floor for visualization
    gl::Texture_2d generate_chequered_floor_texture();

    // flags for loading textures from disk
    enum Tex_flags {
        TexFlag_None = 0,
        TexFlag_SRGB = 1,

        // BEWARE: this flips pixels vertically (in Y) but leaves the pixel's
        // contents untouched. This is fine if the pixels represent colors,
        // but can cause surprising behavior if the pixels represent vectors
        //
        // therefore, if you are flipping (e.g.) normal maps, you may *also* need
        // to flip the pixel content appropriately (e.g. if RGB represents XYZ then
        // you'll need to negate each G)
        TexFlag_Flip_Pixels_Vertically = 2,
    };

    // an image loaded onto the GPU, plus CPU-side metadata (dimensions, channels)
    struct Image_texture final {
        gl::Texture_2d texture;

        // dimensions
        int width;
        int height;

        // in most cases, 3 == RGB, 4 == RGBA
        int channels;
    };

    // read an image file (.PNG, .JPEG, etc.) directly into an OpenGL (GPU) texture
    Image_texture load_image_as_texture(char const* path, Tex_flags = TexFlag_None);

    // read 6 image files into a single OpenGL cubemap (GL_TEXTURE_CUBE_MAP)
    //
    // useful for skyboxes, precomputed point-shadow maps, etc.
    gl::Texture_cubemap load_cubemap(
        char const* path_pos_x,
        char const* path_neg_x,
        char const* path_pos_y,
        char const* path_neg_y,
        char const* path_pos_z,
        char const* path_neg_z);

    // create a normal transform from a model transform matrix
    template<typename Mtx>
    static constexpr glm::mat3 normal_matrix(Mtx&& m) noexcept {
        glm::mat3 top_left{m};
        return glm::inverse(glm::transpose(top_left));
    }

    // raw data that can be serialized through the entire rendering pipeline
    //
    // this enables labelling pixels in the screen. E.g. a pixel could be
    // labelled with an index into a LUT of `OpenSim::Component`s, for selection
    // logic hit-testing
    struct Passthrough_data final {
        GLubyte b0;
        GLubyte b1;
        GLubyte rim_alpha;

        Passthrough_data() = default;

        constexpr Passthrough_data(GLubyte _b0, GLubyte _b1, GLubyte _rim_alpha) noexcept :
            b0{_b0}, b1{_b1}, rim_alpha{_rim_alpha} {
        }

        constexpr void assign_u16(uint16_t v) noexcept {
            b0 = v & 0xff;
            b1 = (v >> 8) & 0xff;
        }

        [[nodiscard]] constexpr uint16_t get_u16() const noexcept {
            uint16_t v = static_cast<uint16_t>(b0);
            v |= static_cast<uint16_t>(b1) << 8;
            return v;
        }
    };
    static_assert(std::is_trivial_v<Rgb24>);
    static_assert(std::is_trivial_v<Passthrough_data>);
    static_assert(sizeof(Rgb24) == sizeof(Passthrough_data));

    // single instance of a mesh
    //
    // the backend renderer supports batched/instanced rendering. This means that
    // it can group mesh instances in such a way that a single OpenGL drawcall draws
    // multiple meshes - this is useful for OpenSim models, which typically contain
    // a lot of repetitive geometry (e.g. spheres in muscles)
    struct alignas(16) Mesh_instance final {
        glm::mat4x3 model_xform;
        glm::mat3 normal_xform;
        Rgba32 rgba;

        union {
            Passthrough_data passthrough;
            Rgb24 passthrough_as_color;
        };

        Instance_flags flags;
        Texidx texidx;
        Meshidx meshidx;

        Mesh_instance() noexcept : passthrough_as_color{0x00, 0x00, 0x00} {
        }

        [[nodiscard]] constexpr bool is_opaque() const noexcept {
            return rgba.a == 0xff || texidx.is_valid();
        }
    };

    // a structured list of mesh instances
    //
    // this structure is optimized with knowledge about GPU instancing - don't
    // directly access the data members because their layout is subject to change
    // as the renderer evolves (e.g. OpenGL4 has many direct memory access APIs
    // that might benefit from switching std::vector for direct pointers into
    // memory-mapped GPU buffers)
    struct Drawlist final {

        // don't screw with these.
        //
        // yes, I know the `private:` keyword, but I can't be assed writing the necessary
        // supporting code to handle the fact that there may be many functions internally
        // inside the renderer that need non-private access to the drawlist
        std::vector<std::vector<Mesh_instance>> _opaque_by_meshidx;
        std::vector<std::vector<Mesh_instance>> _nonopaque_by_meshidx;

        [[nodiscard]] size_t size() const noexcept {
            size_t acc = 0;
            for (auto const& lst : _opaque_by_meshidx) {
                acc += lst.size();
            }
            for (auto const& lst : _nonopaque_by_meshidx) {
                acc += lst.size();
            }
            return acc;
        }

        void push_back(Mesh_instance const& mi) {
            auto& lut = mi.is_opaque() ? _opaque_by_meshidx : _nonopaque_by_meshidx;

            size_t meshidx = mi.meshidx.as_index();

            size_t minsize = meshidx + 1;
            if (lut.size() < minsize) {
                lut.resize(minsize);
            }

            lut[meshidx].push_back(mi);
        }

        void clear() {
            // don't clear the top-level vectors, so that heap recyling works.

            for (auto& lst : _opaque_by_meshidx) {
                lst.clear();
            }
            for (auto& lst : _nonopaque_by_meshidx) {
                lst.clear();
            }
        }

        template<typename Callback>
        void for_each(Callback f) {
            for (auto& lst : _opaque_by_meshidx) {
                for (auto& mi : lst) {
                    f(mi);
                }
            }
            for (auto& lst : _nonopaque_by_meshidx) {
                for (auto& mi : lst) {
                    f(mi);
                }
            }
        }

        template<typename Callback>
        void for_each(Callback f) const {
            for (auto const& lst : _opaque_by_meshidx) {
                for (auto const& mi : lst) {
                    f(mi);
                }
            }
            for (auto const& lst : _nonopaque_by_meshidx) {
                for (auto const& mi : lst) {
                    f(mi);
                }
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
    // this "uploads" a CPU-side mesh onto the GPU in a format that the renderer
    // knows how to use
    struct GPU_mesh final {
        gl::Array_buffer<GLubyte> verts;
        gl::Element_array_buffer<elidx_t> indices;
        gl::Array_buffer<Mesh_instance, GL_DYNAMIC_DRAW> instances;
        gl::Vertex_array main_vao;
        gl::Vertex_array normal_vao;
        bool is_textured;

        GPU_mesh(Untextured_mesh const&);
        GPU_mesh(Textured_mesh const&);
    };

    // shader forward-decls
    //
    // include the shader's header if you need to actually access these
    struct Gouraud_mrt_shader;
    struct Normals_shader;
    struct Plain_texture_shader;
    struct Colormapped_plain_texture_shader;
    struct Edge_detection_shader;
    struct Skip_msxaa_blitter_shader;

    // aggregate of elements in GPU storage
    //
    // this is a single class that just holds everything that's been uploaded
    // to the GPU. It's utility is that it provides a one-stop shop to grab
    // GPU data
    struct GPU_storage final {
        // initialized shaders
        std::unique_ptr<Gouraud_mrt_shader> shader_gouraud;
        std::unique_ptr<Normals_shader> shader_normals;
        std::unique_ptr<Plain_texture_shader> shader_pts;
        std::unique_ptr<Colormapped_plain_texture_shader> shader_cpts;
        std::unique_ptr<Edge_detection_shader> shader_eds;
        std::unique_ptr<Skip_msxaa_blitter_shader> shader_skip_msxaa;

        // raw mesh storage: `Meshidx` is effectively an index into this
        std::vector<GPU_mesh> meshes;

        // raw texture storage: `Texidx` is effectively an index into this
        std::vector<gl::Texture_2d> textures;

        // maps filesystem paths (e.g. .VTP files) to mesh indexes that are
        // already loaded on the GPU
        //
        // used for runtime data caching
        std::unordered_map<std::string, Meshidx> path_to_meshidx;

        // preallocated meshes
        //
        // the ctor will automatically populate these appropriately, so
        // downstream code can just use the indices directly
        Meshidx simbody_sphere_idx;
        Meshidx simbody_cylinder_idx;
        Meshidx simbody_cube_idx;
        Meshidx floor_quad_idx;
        Meshidx grid_25x25_idx;
        Meshidx yline_idx;
        Meshidx cube_lines_idx;
        Meshidx quad_idx;

        // preallocated textures
        Texidx chequer_idx;

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

    // output target for the renderer
    //
    // these are what the renderer writes to. Most of these are internal
    // details but are exposed anyway because it's useful to view them
    // (e.g. debugging)
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

        // create a "blank" render target
        //
        // callers should `reconfigure` this before use
        Render_target() : Render_target{1, 1, 1} {}

        // create a valid render target with specified dimensions + samples
        Render_target(int w, int h, int samples);

        // will reinitialize the buffers accordingly
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

    // flags for a renderer drawcall
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
        DrawcallFlags_DrawDebugQuads = 1 << 3,

        // perform hit testing on Raw_mesh_instance passthrough data
        DrawcallFlags_PerformPassthroughHitTest = 1 << 4,

        // use optimized hit testing (which might arrive a frame late)
        DrawcallFlags_UseOptimizedButDelayed1FrameHitTest = 1 << 5,

        // draw the scene
        DrawcallFlags_DrawSceneGeometry = 1 << 6,

        // use instanced (optimized) rendering
        DrawcallFlags_UseInstancedRenderer = 1 << 7,

        DrawcallFlags_Default = DrawcallFlags_DrawRims |
                                DrawcallFlags_PerformPassthroughHitTest |
                                DrawcallFlags_UseOptimizedButDelayed1FrameHitTest |
                                DrawcallFlags_DrawSceneGeometry |
                                DrawcallFlags_UseInstancedRenderer
    };

    // parameters for a renderer drawcall
    struct Render_params final {
        // worldspace -> viewspace transform matrix
        glm::mat4 view_matrix = glm::mat4{1.0f};

        // viewspace -> clipspace transform matrix
        glm::mat4 projection_matrix = glm::mat4{1.0f};

        // worldspace position of the viewer
        glm::vec3 view_pos = {0.0f, 0.0f, 0.0f};

        // worldspace direction of the light
        glm::vec3 light_dir = {-0.34f, -0.25f, 0.05f};

        // rgb color of the light
        glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};

        // solid background color
        glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};

        // color of any rim highlights
        glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 0.85f};

        // screenspace coordinates for hit testing
        //
        // - the renderer will write the hittest result to the Render_target
        // - negative values indicate "skip testing"
        // - the hittest result is whatever passthrough_data was set in the
        //   mesh instances in the drawlist
        // - e.g. set the passthrough_data to an ID, set these coords to mouse
        //   location, then you can figure out which ID is under the mouse
        glm::ivec2 hittest = {-1, -1};

        // any modal flags to apply during the rendering process
        DrawcallFlags flags = DrawcallFlags_Default;
    };

    // draw a scene into the specified render target
    //
    // - GPU_storage is mutable because the renderer might mutate instanced mesh data
    void draw_scene(GPU_storage&, Render_params const&, Drawlist const&, Render_target&);
}
