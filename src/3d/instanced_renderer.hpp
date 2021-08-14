#pragma once

#include "src/3d/gl.hpp"
#include "src/3d/3d.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

#include <memory>
#include <vector>
#include <atomic>
#include <unordered_map>

namespace osc {
    struct Mesh_instance final {
        glm::mat4x3 model_xform;  // model --> world xform
        glm::mat3 normal_xform;  // normal xform for the above
        Rgba32 rgba;  // color, if untextured
        GLushort meshidx;  // index into meshes array
        GLshort texidx;  // index into textures array, or -1 if untextured
        GLubyte rim_intensity;  // intensity of rim highlight (0x00 for no highlit, 0xff for full highlight)
    };

    // generate a new, globally-unique for all Ts, ID for an item of type T
    template<typename T>
    inline int make_id() {
        static std::atomic<int> g_NextId = 0;
        return g_NextId;
    }

    struct Mesh_instance_meshdata final {
        gl::Array_buffer<GLubyte> verts;  // raw vertex data on GPU
        gl::Element_array_buffer<GLushort> indices;  // vertex indices on GPU
        gl::Array_buffer<Mesh_instance, GL_DYNAMIC_DRAW> instances;  // per-instance data on GPU
        std::unordered_map<int, gl::Vertex_array> vaos;  // associative id-to-VAO storage
        bool is_textured;  // true if `verts` contains `Textured_vert`, false if `Untextured_vert`

        explicit Mesh_instance_meshdata(Untextured_mesh const&);
        explicit Mesh_instance_meshdata(Textured_mesh const&);
    };

    struct Mesh_instance_drawlist final {
        std::vector<Mesh_instance> instances;  // instances to render
        std::vector<std::shared_ptr<Mesh_instance_meshdata>> meshes;  // mesh data associated to each instance via meshidx
        std::vector<std::shared_ptr<gl::Texture_2d>> textures;  // textures associated to each instance via texidx
    };

    // flags for a drawcall
    using DrawcallFlags = int;
    enum DrawcallFlags_ {
        DrawcallFlags_None = 0 << 0,
        DrawcallFlags_WireframeMode = 1 << 0,  // render in wireframe mode
        DrawcallFlags_ShowMeshNormals = 1 << 1,  // render mesh normals
        DrawcallFlags_DrawRims = 1 << 2,  // render rim highlights
        DrawcallFlags_DrawDebugQuads = 1 << 3,  // render debug quads (development)
        DrawcallFlags_DrawSceneGeometry = 1 << 6,  // render the scene
        DrawcallFlags_UseInstancedRenderer = 1 << 7,  // disable instanced rendering (development)

        DrawcallFlags_Default = DrawcallFlags_DrawRims | DrawcallFlags_DrawSceneGeometry | DrawcallFlags_UseInstancedRenderer
    };

    // parameters (incl. flags) for a drawcall
    struct Render_params final {
        glm::mat4 view_matrix = glm::mat4{1.0f};  // worldspace -> viewspace transform matrix
        glm::mat4 projection_matrix = glm::mat4{1.0f};  // viewspace -> clipspace transform matrix
        glm::vec3 view_pos = {0.0f, 0.0f, 0.0f};  // worldspace position of the viewer
        glm::vec3 light_dir = {-0.34f, -0.25f, 0.05f};  // worldspace direction of the light
        glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};  // rgb color of the light
        glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};  // solid background color
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

        [[nodiscard]] glm::ivec2 dims() const noexcept;
        [[nodiscard]] glm::vec2 dimsf() const noexcept;
        void set_dims(glm::ivec2);

        [[nodiscard]] float aspect_ratio() const noexcept;

        [[nodiscard]] int msxaa_samples() const noexcept;
        void set_msxaa_samples(int);

        // draws scene and updates the textures returned by the calls below
        //
        // note:
        //   - can mutate the order of `instances` in the drawlist
        //   - can mutate the VAOs stored in meshdata stored in the drawlist
        void render(Render_params const&, Mesh_instance_drawlist&);

        gl::Texture_2d const& output_texture() const noexcept;
        gl::Texture_2d& output_texture() noexcept;

        gl::Texture_2d const& depth_texture() const noexcept;
        gl::Texture_2d& depth_texture();
    };
}
