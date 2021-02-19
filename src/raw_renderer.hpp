#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstddef>

// raw renderer: an OpenGL renderer that is Application, Screen, and OpenSim agnostic.
//
// this API is designed with performance and power in mind, not convenience. Use a downstream
// renderer (e.g. a specialized OpenSim model renderer) if you need something more convenient.
namespace gl {
    struct Texture_2d;
}

namespace osmv {
    constexpr int invalid_meshid = -1;

    // globally allocate mesh data on the GPU
    //
    // the returned handle is a "mesh ID" and is guaranteed to be a non-negative number that
    // increases monotonically
    //
    // must only be called after OpenGL is initialized
    struct Untextured_vert;
    int globally_allocate_mesh(osmv::Untextured_vert const* verts, size_t n);

    struct Rgba32 final {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;

        Rgba32() = default;

        Rgba32(glm::vec4 const& v) :
            r{static_cast<unsigned char>(255.0f * v.r)},
            g{static_cast<unsigned char>(255.0f * v.g)},
            b{static_cast<unsigned char>(255.0f * v.b)},
            a{static_cast<unsigned char>(255.0f * v.a)} {
        }
    };

    struct Passthrough_data final {
        unsigned char b0;
        unsigned char b1;

        static constexpr Passthrough_data from_u16(uint16_t v) noexcept {
            unsigned char b0 = v & 0xff;
            unsigned char b1 = (v >> 8) & 0xff;
            return Passthrough_data{b0, b1};
        }

        constexpr uint16_t to_u16() const noexcept {
            uint16_t rv = b0;
            rv |= static_cast<uint16_t>(b1) << 8;
            return rv;
        }
    };

    // one instance of a mesh
    //
    // this struct is fairly complicated because it has to pack data together ready for a
    // GPU draw call. Instanced GPU drawing requires that the data is contiguous and has all
    // necessary draw parameters (transform matrices, etc.) at predictable memory offsets.
    struct alignas(16) Mesh_instance final {
        // transforms mesh vertices into scene worldspace
        glm::mat4 transform;

        // INTERNAL: normal transform: transforms mesh normals into scene worldspace
        //
        // this is mostly here as a draw-time optimization because it is redundant to compute
        // it every draw call (and because instanced rendering requires this to be available
        // in this struct)
        glm::mat3 _normal_xform;

        // primary mesh RGBA color
        //
        // this color is subject to mesh shading (lighting, shadows), so the rendered color may
        // differ
        //
        // note: alpha blending can be expensive. You should try to keep geometry opaque,
        //       unless you *really* need blending
        Rgba32 rgba;

        // INTERNAL: passthrough data
        //
        // this is used internally by the renderer to pass data between shaders, enabling
        // screen-space logic (selection logic, rim highlights, etc.)
        //
        // currently used for:
        //
        //     - r+g: raw passthrough data, used to handle selection logic. Downstream renderers
        //             use these channels to encode logical information (e.g. "an OpenSim component")
        //             into screen-space (e.g. "A pixel from an OpenSim component")
        //
        //     - b:    unused (reserved)
        //
        //     - a:    rim alpha. Used to calculate how strongly (if any) rims should be drawn
        //             around the rendered geometry. Used for highlighting elements in the scene
        Rgba32 _passthrough;

        // INTERNAL: mesh ID: globally unique ID for the mesh vertices that should be rendered
        //
        // the renderer uses this ID to deduplicate and instance draw calls. You shouldn't mess
        // with this unless you know what you're doing
        int _meshid;

        // trivial ctor: useful if the caller knows what they're doing and some STL
        //               algorithms like when a type is trivially constructable
        Mesh_instance() = default;

        template<typename Mat4, typename Rgba>
        Mesh_instance(Mat4&& _transform, Rgba&& _rgba, int meshid) noexcept :
            transform{std::forward<Mat4>(_transform)},
            _normal_xform{glm::transpose(glm::inverse(transform))},
            rgba{std::forward<Rgba>(_rgba)},
            _passthrough{},
            _meshid{meshid} {
        }

        void set_rim_alpha(unsigned char a) noexcept {
            _passthrough.a = a;
        }

        // set passthrough data
        //
        // note: wherever the scene *isn't* rendered, black (0x000000) is encoded, so users of
        //       this should treat 0x000000 as "reserved"
        void set_passthrough_data(Passthrough_data pd) noexcept {
            _passthrough.r = pd.b0;
            _passthrough.g = pd.b1;
        }

        Passthrough_data passthrough_data() const noexcept {
            return {_passthrough.r, _passthrough.g};
        }
    };

    // reorder a contiguous sequence of mesh instances for optimal drawing
    void optimize_draw_order(Mesh_instance* begin, size_t) noexcept;

    struct Raw_renderer_config final {
        int w;
        int h;
        int samples;
    };

    using Raw_renderer_flags = int;
    enum Raw_renderer_flags_ {
        RawRendererFlags_None = 0 << 0,
        RawRendererFlags_WireframeMode = 1 << 0,
        RawRendererFlags_ShowMeshNormals = 1 << 1,
        RawRendererFlags_ShowFloor = 1 << 2,
        RawRendererFlags_DrawRims = 1 << 3,
        RawRendererFlags_DrawDebugQuads = 1 << 4,
        RawRendererFlags_PerformPassthroughHitTest = 1 << 5,
        RawRendererFlags_UseOptimizedButDelayed1FrameHitTest = 1 << 6,
        RawRendererFlags_DrawSceneGeometry = 1 << 7,

        RawRendererFlags_Default = RawRendererFlags_ShowFloor | RawRendererFlags_DrawRims |
                                   RawRendererFlags_DrawDebugQuads | RawRendererFlags_PerformPassthroughHitTest |
                                   RawRendererFlags_UseOptimizedButDelayed1FrameHitTest |
                                   RawRendererFlags_DrawSceneGeometry
    };

    struct Raw_drawcall_params final {
        glm::mat4 view_matrix = {};
        glm::mat4 projection_matrix = {};
        glm::vec3 view_pos = {};
        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
        glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};
        glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 1.0f};
        float rim_thickness = 0.002f;

        Raw_renderer_flags flags = RawRendererFlags_Default;
        int passthrough_hittest_x = 0;
        int passthrough_hittest_y = 0;
    };

    struct Raw_drawcall_result final {
        gl::Texture_2d& texture;
        Passthrough_data passthrough_result;
    };

    struct Renderer_impl;
    class Raw_renderer final {
        Renderer_impl* impl;

    public:
        Raw_renderer(Raw_renderer_config const&);
        Raw_renderer(Raw_renderer const&) = delete;
        Raw_renderer(Raw_renderer&&) = delete;
        Raw_renderer& operator=(Raw_renderer const&) = delete;
        Raw_renderer& operator=(Raw_renderer&&) = delete;
        ~Raw_renderer() noexcept;

        void change_config(Raw_renderer_config const&);
        glm::vec2 dimensions() const noexcept;
        float aspect_ratio() const noexcept;

        Raw_drawcall_result draw(Raw_drawcall_params const&, Mesh_instance const* begin, size_t n);

        template<typename ContiguousContainer>
        Raw_drawcall_result draw(Raw_drawcall_params const& params, ContiguousContainer const& c) {
            return draw(params, c.data(), c.size());
        }
    };
}
