#pragma once

#include <glm/mat4x4.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <cstddef>

namespace gl {
    class Texture_2d;
    class Frame_buffer;
}

namespace osc {
    struct NewMesh;
    struct Rgba32;
}

namespace osc {

    struct Drawlist_compiler_input;
    class Instanced_drawlist;

    // opaque handle to meshdata that has been uploaded to the backend
    class Instanceable_meshdata final {
    public:
        struct Impl;
    private:
        friend class Instanced_renderer;
        friend Instanceable_meshdata upload_meshdata_for_instancing(NewMesh const&);
        friend void upload_inputs_to_drawlist(Drawlist_compiler_input const&, Instanced_drawlist&);
        std::shared_ptr<Impl> m_Impl;
        Instanceable_meshdata(std::shared_ptr<Impl>);
    public:
        ~Instanceable_meshdata() noexcept;
    };

    // uploads mesh data to the backend
    Instanceable_meshdata upload_meshdata_for_instancing(NewMesh const&);

    // data inputs the backend needs to generate an instance drawlist
    struct Drawlist_compiler_input final {
        size_t ninstances = 0;
        glm::mat4x3 const* model_xforms = nullptr;
        glm::mat3 const* normal_xforms = nullptr;
        Rgba32 const* colors = nullptr;
        Instanceable_meshdata const* meshes = nullptr;
        std::shared_ptr<gl::Texture_2d> const* textures = nullptr;
        unsigned char const* rim_intensity = nullptr;
    };

    // opaque handle to a drawlist the backend can render rapidly
    class Instanced_drawlist final {
    public:
        struct Impl;
    private:
        friend class Instanced_renderer;
        friend void upload_inputs_to_drawlist(Drawlist_compiler_input const&, Instanced_drawlist&);
        std::shared_ptr<Impl> m_Impl;
    public:
        Instanced_drawlist();
        ~Instanced_drawlist() noexcept;
    };

    // writes inputs into the drawlist
    void upload_inputs_to_drawlist(Drawlist_compiler_input const&, Instanced_drawlist&);

    // flags for a render drawcall
    using DrawcallFlags = int;
    enum DrawcallFlags_ {
        DrawcallFlags_None = 0 << 0,
        DrawcallFlags_WireframeMode = 1 << 0,  // render in wireframe mode
        DrawcallFlags_ShowMeshNormals = 1 << 1,  // render mesh normals
        DrawcallFlags_DrawRims = 1 << 2,  // render rim highlights
        DrawcallFlags_DrawSceneGeometry = 1 << 3,  // render the scene (development)

        DrawcallFlags_Default = DrawcallFlags_DrawRims | DrawcallFlags_DrawSceneGeometry
    };

    // parameters for a render drawcall
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
        void render(Render_params const&, Instanced_drawlist const&);

        gl::Frame_buffer const& output_fbo() const noexcept;
        gl::Frame_buffer& output_fbo() noexcept;

        gl::Texture_2d const& output_texture() const noexcept;
        gl::Texture_2d& output_texture() noexcept;

        gl::Texture_2d const& depth_texture() const noexcept;
        gl::Texture_2d& depth_texture();
    };
}
