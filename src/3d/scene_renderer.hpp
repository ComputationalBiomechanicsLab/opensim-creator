#pragma once

#include "raw_mesh_instance.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <memory>

namespace gl {
    struct Texture_2d;
}

namespace osmv {
    struct Untextured_vert;
    struct Textured_vert;
    class Raw_drawlist;
    struct Mesh_on_gpu;
    struct Gpu_storage;
}

namespace sdl {
    class GLContext;
}
// raw renderer: an OpenGL renderer that is Application, Screen, and OpenSim agnostic.
//
// this API is designed with performance and power in mind, not convenience. Use a downstream
// renderer (e.g. a specialized OpenSim model renderer) if you need something more convenient.
namespace osmv {

    struct Raw_renderer_config final {
        int w;
        int h;
        int samples;
    };

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

        RawRendererFlags_Default =
            DrawcallFlags_DrawRims | RawRendererFlags_DrawDebugQuads | RawRendererFlags_PerformPassthroughHitTest |
            RawRendererFlags_UseOptimizedButDelayed1FrameHitTest | RawRendererFlags_DrawSceneGeometry
    };

    struct Raw_drawcall_params final {
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

    struct Raw_drawcall_result final {
        gl::Texture_2d& texture;
        Passthrough_data passthrough_result;
    };

    class Raw_renderer final {
        class Impl;
        std::unique_ptr<Impl> impl;

    public:
        Raw_renderer(Raw_renderer_config const&);
        ~Raw_renderer() noexcept;

        void change_config(Raw_renderer_config const&);
        glm::vec2 dimensions() const noexcept;
        float aspect_ratio() const noexcept;

        Raw_drawcall_result draw(Gpu_storage const&, Raw_drawcall_params const&, Raw_drawlist const&);
    };
}
