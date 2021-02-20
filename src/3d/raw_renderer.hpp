#pragma once

#include "raw_drawlist.hpp"
#include "raw_mesh_instance.hpp"

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

        Raw_drawcall_result draw(Raw_drawcall_params const&, Raw_drawlist const&);
    };
}
