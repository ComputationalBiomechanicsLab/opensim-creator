#pragma once

#include "src/3d/mesh_instance.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstddef>

namespace gl {
    class Texture_2d;
}

namespace osc {
    struct Untextured_vert;
    struct Textured_vert;
    class Drawlist;
    struct Mesh_on_gpu;
    struct Gpu_storage;
    class Render_target;
}

namespace sdl {
    class GLContext;
}
// raw renderer: an OpenGL renderer that is Application, Screen, and OpenSim agnostic.
//
// this API is designed with performance and power in mind, not convenience. Use a downstream
// renderer (e.g. a specialized OpenSim model renderer) if you need something more convenient.
namespace osc {

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

    class Renderer final {
        struct Impl;
        Impl* impl;

    public:
        Renderer();
        Renderer(Renderer const&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer const&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        ~Renderer() noexcept;

        [[nodiscard]] Passthrough_data
            draw(Gpu_storage const&, Raw_drawcall_params const&, Drawlist const&, Render_target& out);
    };
}
