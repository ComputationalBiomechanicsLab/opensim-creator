#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstddef>

// raw renderer: an OpenGL renderer that is Application, Screen, and OpenSim agnostic.
//
// this API is designed with performance and power in mind, not convenience. Use a downstream
// renderer (e.g. a specialized OpenSim model renderer) if you need something more convenient.
namespace osmv {
    constexpr int invalid_meshid = -1;

    // globally allocate mesh data on the GPU
    //
    // the returned handle is a "mesh ID" and is guaranteed to be a positive number that
    // increases monotonically
    //
    // must only be called after OpenGL is initialized
    struct Untextured_vert;
    int globally_allocate_mesh(osmv::Untextured_vert const* verts, size_t n);

    // one instance of a mesh
    //
    // this struct is fairly complicated because it has to pack data together ready for a
    // GPU draw call. Instanced GPU drawing requires that the data is contiguous and has all
    // necessary draw parameters (transform matrices, etc.) at predictable memory offsets.
    struct Mesh_instance final {

        // transforms mesh vertices into scene worldspace
        glm::mat4 transform;

        // primary mesh RGBA color
        //
        // this color is subject to mesh shading (lighting, shadows), so the rendered color may
        // differ
        //
        // note: alpha blending can be expensive. You should try to keep alpha >= 1.0f
        //       unless you *really* need blending
        glm::vec4 rgba;

        // INTERNAL: passthrough RGBA color
        //
        // this color is guaranteed to be drawn "as-is" to the COLOR1 output with no shading,
        // which enables downstream screen-space calculations (selection logic, rim highlights)
        //
        // currently used for:
        //
        //     - RG: raw passthrough data, used to handle selection logic. Downstream renderers
        //           use these channels to encode logical information (e.g. "an OpenSim component")
        //           into screen-space (e.g. "A pixel from an OpenSim component")
        //
        //     - B : currently unused
        //
        //     - A : rim alpha. Used to calculate how strongly (if any) rims should be drawn
        //           around the geometry. Used for highlighting elements in the scene
        glm::vec4 _passthrough = glm::vec4{0.0f, 0.0f, 0.0f, 0.0f};

        // INTERNAL: normal transform: transforms mesh normals into scene worldspace
        //
        // this is mostly here as a draw-time optimization because it is redundant to compute
        // it every draw call (and because instanced rendering requires this to be available
        // in this struct)
        //
        // you can regenerate this with:
        //     glm::transpose(glm::inverse(transform));
        // or:
        //     gl::normal_matrix(transform);
        glm::mat4 _normal_xform;

        // INTERNAL: mesh ID: globally unique ID for the mesh vertices that should be rendered
        //
        // the renderer uses this ID to deduplicate and instance draw calls. You shouldn't mess
        // with this unless you know what you're doing.
        int _meshid;

        Mesh_instance(glm::mat4 const& _transform, glm::vec4 const& _rgba, int __meshid) :
            transform{_transform},
            rgba{_rgba},
            _normal_xform{glm::transpose(glm::inverse(transform))},
            _meshid{__meshid} {
        }

        void set_rim_alpha(float a) {
            _passthrough.a = a;
        }

        // set passthrough data
        //
        // note: wherever the scene *isn't* rendered, black (0x000000) is encoded, so users of
        //       this should treat 0x000000 as "reserved"
        void set_passthrough_data(unsigned char b0, unsigned char b1) {
            // map a byte range (0 - 255) onto an OpenGL color range (0.0f - 1.0f)
            _passthrough.r = static_cast<float>(b0) / 255.0f;
            _passthrough.g = static_cast<float>(b1) / 255.0f;
        }
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

    struct Renderer_impl;
    struct Raw_renderer final {
        glm::mat4 view_matrix{};
        glm::mat4 projection_matrix{};
        glm::vec3 view_pos = {0.0f, 0.0f, 0.0f};
        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
        glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};
        glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 1.0f};
        float rim_thickness = 0.002f;
        Raw_renderer_flags flags = RawRendererFlags_Default;
        int passthrough_hittest_x = 0;
        int passthrough_hittest_y = 0;
        unsigned char passthrough_result_prev_frame[2];
        unsigned char passthrough_result_this_frame[2];

    private:
        Renderer_impl* state;

    public:
        Raw_renderer(int w, int h, int samples);
        Raw_renderer(Raw_renderer const&) = delete;
        Raw_renderer(Raw_renderer&&) = delete;
        Raw_renderer& operator=(Raw_renderer const&) = delete;
        Raw_renderer& operator=(Raw_renderer&&) = delete;
        ~Raw_renderer() noexcept;

        void reallocate_buffers(int w, int h, int samples);

        // sort the provided meshes ready for a draw call
        //
        // if you skip this step, drawing might perform *extremely* sub-optimally and
        // blended components might be drawn in the wrong order
        void sort_meshes_for_drawing(Mesh_instance* meshes, size_t n);

        template<typename Container>
        void sort_meshes_for_drawing(Container& c) {
            sort_meshes_for_drawing(c.data(), c.size());
        }

        void draw(Mesh_instance const* meshes, size_t n);

        template<typename Container>
        void draw(Container const& c) {
            draw(c.data(), c.size());
        }
    };
}
