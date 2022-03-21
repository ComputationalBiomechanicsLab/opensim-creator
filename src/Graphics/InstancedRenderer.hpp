#pragma once

#include <glm/mat4x4.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <cstddef>

namespace gl
{
    class Texture2D;
    class FrameBuffer;
}

namespace osc
{
    struct MeshData;
    struct Rgba32;
}

namespace osc
{
    struct DrawlistCompilerInput;
    class InstancedDrawlist;

    // opaque handle to meshdata that has been uploaded to the backend
    class InstanceableMeshdata final {
    public:
        struct Impl;
    private:
        friend class InstancedRenderer;
        friend InstanceableMeshdata uploadMeshdataForInstancing(MeshData const&);
        friend void uploadInputsToDrawlist(DrawlistCompilerInput const&, InstancedDrawlist&);
        std::shared_ptr<Impl> m_Impl;
        InstanceableMeshdata(std::shared_ptr<Impl>);
    public:
        ~InstanceableMeshdata() noexcept;
    };

    // uploads mesh data to the backend
    InstanceableMeshdata uploadMeshdataForInstancing(MeshData const&);

    // data inputs the backend needs to generate an instance drawlist
    struct DrawlistCompilerInput final {
        size_t ninstances = 0;
        glm::mat4x3 const* modelMtxs = nullptr;
        glm::mat3 const* normalMtxs = nullptr;
        Rgba32 const* colors = nullptr;
        InstanceableMeshdata const* meshes = nullptr;
        std::shared_ptr<gl::Texture2D> const* textures = nullptr;
        unsigned char const* rimIntensities = nullptr;
    };

    // opaque handle to a drawlist the backend can render rapidly
    class InstancedDrawlist final {
    public:
        struct Impl;
    private:
        friend class InstancedRenderer;
        friend void uploadInputsToDrawlist(DrawlistCompilerInput const&, InstancedDrawlist&);
        std::shared_ptr<Impl> m_Impl;
    public:
        InstancedDrawlist();
        ~InstancedDrawlist() noexcept;
    };

    // writes inputs into the drawlist
    void uploadInputsToDrawlist(DrawlistCompilerInput const&, InstancedDrawlist&);

    // flags for a render drawcall
    using InstancedRendererFlags = int;
    enum InstancedRendererFlags_ {
        InstancedRendererFlags_None = 0 << 0,
        InstancedRendererFlags_WireframeMode = 1 << 0,  // render in wireframe mode
        InstancedRendererFlags_ShowMeshNormals = 1 << 1,  // render mesh normals
        InstancedRendererFlags_DrawRims = 1 << 2,  // render rim highlights
        InstancedRendererFlags_DrawSceneGeometry = 1 << 3,  // render the scene (development)

        InstancedRendererFlags_Default = InstancedRendererFlags_DrawRims | InstancedRendererFlags_DrawSceneGeometry
    };

    // parameters for a render drawcall
    struct InstancedRendererParams final {
        glm::mat4 viewMtx = glm::mat4{1.0f};  // worldspace -> viewspace transform matrix
        glm::mat4 projMtx = glm::mat4{1.0f};  // viewspace -> clipspace transform matrix
        glm::vec3 viewPos = {0.0f, 0.0f, 0.0f};  // worldspace position of the viewer
        glm::vec3 lightDir = {-0.34f, -0.25f, 0.05f};  // worldspace direction of the directional light
        glm::vec3 lightCol = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};  // rgb color of the directional light
        glm::vec4 backgroundCol = {0.89f, 0.89f, 0.89f, 1.0f};  // what the framebuffer will be cleared with pre-render
        glm::vec4 rimCol = {1.0f, 0.4f, 0.0f, 0.85f};  // color of any rim highlights
        InstancedRendererFlags flags = InstancedRendererFlags_Default;  // flags
    };

    class InstancedRenderer final {
    public:
        InstancedRenderer();
        InstancedRenderer(glm::ivec2 dims, int samples);
        ~InstancedRenderer() noexcept;

        [[nodiscard]] glm::ivec2 getDims() const noexcept;
        [[nodiscard]] glm::vec2 getDimsf() const noexcept;
        void setDims(glm::ivec2);

        [[nodiscard]] float getAspectRatio() const noexcept;

        [[nodiscard]] int getMsxaaSamples() const noexcept;
        void setMsxaaSamples(int);

        // render the scene to the output texture
        //
        // note: optimal performance depends on the ordering of instances in the drawlist
        //       see comment next to the drawlist
        void render(InstancedRendererParams const&, InstancedDrawlist const&);

        gl::FrameBuffer const& getOutputFbo() const noexcept;
        gl::FrameBuffer& getOutputFbo() noexcept;

        gl::Texture2D const& getOutputTexture() const noexcept;
        gl::Texture2D& getOutputTexture() noexcept;

        gl::Texture2D const& getOutputDepthTexture() const noexcept;
        gl::Texture2D& getOutputDepthTexture();

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
