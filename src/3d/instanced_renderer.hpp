#pragma once

#include "src/3d/gl.hpp"
#include "src/3d/model.hpp"

#include <glm/mat4x4.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <vector>

namespace osc {
    // instance of a mesh to render
    //
    // there can be *many* (e.g. hundreds of thousands) of these in a single drawcall
    struct alignas(32) Mesh_instance final {
        glm::mat4x3 model_xform;  // model --> world xform
        glm::mat3 normal_xform;  // normal xform for the above
        Rgba32 rgba;  // color, if untextured
        GLushort meshidx;  // index into meshes array
        GLshort texidx;  // index into textures array, or -1 if untextured
        GLubyte rim_intensity;  // intensity of rim highlight (0x00 for no highlit, 0xff for full highlight)
        short data;  // not used by the renderer - can be used by downstream for intrusive storage
    };

    // handle to instanced-renderer-side data (VAOs, VBOs, etc.)
    //
    // these are shared by mesh instances via the index
    class Refcounted_instance_meshdata final {
    public:
        struct Impl;
    private:
        friend class Instanced_renderer;
        std::shared_ptr<Impl> m_Impl;
        Refcounted_instance_meshdata(std::shared_ptr<Impl>);
    public:
        ~Refcounted_instance_meshdata() noexcept;
    };

    // list of instances to render
    //
    // instances are drawn in the supplied order. The instanced renderer automatically performs
    // a front-to-back O(1) batching based on `meshidx` and `texidx`. You should sort the instance
    // list by these *but* be sure to handle non-opaque objects correctly (draw them last,
    // back-to-front)
    struct Mesh_instance_drawlist final {
        std::vector<Mesh_instance> instances;
        std::vector<Refcounted_instance_meshdata> meshes;  // instance.meshidx can index this
        std::vector<std::shared_ptr<gl::Texture_2d>> textures;  // instance.texidx can index this

        void clear();
    };

    // flags for a drawcall
    using DrawcallFlags = int;
    enum DrawcallFlags_ {
        DrawcallFlags_None = 0 << 0,
        DrawcallFlags_WireframeMode = 1 << 0,  // render in wireframe mode
        DrawcallFlags_ShowMeshNormals = 1 << 1,  // render mesh normals
        DrawcallFlags_DrawRims = 1 << 2,  // render rim highlights
        DrawcallFlags_DrawSceneGeometry = 1 << 3,  // render the scene (development)
        DrawcallFlags_UseInstancedRenderer = 1 << 4,  // disable instanced rendering (development)

        DrawcallFlags_Default = DrawcallFlags_DrawRims | DrawcallFlags_DrawSceneGeometry | DrawcallFlags_UseInstancedRenderer
    };

    // parameters (incl. flags) for a drawcall
    struct Render_params final {
        glm::mat4 view_matrix = glm::mat4{1.0f};  // worldspace -> viewspace transform matrix
        glm::mat4 projection_matrix = glm::mat4{1.0f};  // viewspace -> clipspace transform matrix
        glm::vec3 view_pos = {0.0f, 0.0f, 0.0f};  // worldspace position of the viewer
        glm::vec3 light_dir = {-0.34f, -0.25f, 0.05f};  // worldspace direction of the directional light
        glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};  // rgb color of the directional light
        glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};  // what the framebuffer will be cleared with pre-render
        glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 0.85f};  // color of any rim highlights
        DrawcallFlags flags = DrawcallFlags_Default;  // flags
    };

    class Instanced_renderer final {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        Instanced_renderer();
        Instanced_renderer(glm::ivec2 dims, int samples);
        ~Instanced_renderer() noexcept;

        Refcounted_instance_meshdata allocate(NewMesh const&);  // allocate meshdata on GPU

        [[nodiscard]] glm::ivec2 dims() const noexcept;
        [[nodiscard]] glm::vec2 dimsf() const noexcept;
        void set_dims(glm::ivec2);

        [[nodiscard]] float aspect_ratio() const noexcept;

        [[nodiscard]] int msxaa_samples() const noexcept;
        void set_msxaa_samples(int);

        // render the scene to the output texture
        //
        // note: optimal performance depends on the ordering of instances in the drawlist
        //       see comment next to the drawlist
        void render(Render_params const&, Mesh_instance_drawlist const&);

        gl::Texture_2d const& output_texture() const noexcept;
        gl::Texture_2d& output_texture() noexcept;

        gl::Texture_2d const& depth_texture() const noexcept;
        gl::Texture_2d& depth_texture();
    };
}
