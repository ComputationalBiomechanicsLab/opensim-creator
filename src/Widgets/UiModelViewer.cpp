#include "UiModelViewer.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Shaders/EdgeDetectionShader.hpp"
#include "src/Graphics/Shaders/GouraudShader.hpp"
#include "src/Graphics/Shaders/InstancedGouraudColorShader.hpp"
#include "src/Graphics/Shaders/InstancedSolidColorShader.hpp"
#include "src/Graphics/Shaders/NormalsShader.hpp"
#include "src/Graphics/Shaders/SolidColorShader.hpp"
#include "src/Graphics/BasicSceneElement.hpp"
#include "src/Graphics/DAEWriter.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"
#include "src/Graphics/Texturing.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/RayCollision.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/ComponentDecoration.hpp"
#include "src/OpenSimBindings/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/MuscleColoringStyle.hpp"
#include "src/OpenSimBindings/MuscleDecorationStyle.hpp"
#include "src/OpenSimBindings/MuscleSizingStyle.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/os.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"

#include <GL/glew.h>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_scancode.h>
#include <IconsFontAwesome5.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>


// camera utils
namespace
{
    void FocusAlongX(osc::PolarPerspectiveCamera& camera)
    {
        camera.theta = osc::fpi2;
        camera.phi = 0.0f;
    }

    void FocusAlongMinusX(osc::PolarPerspectiveCamera& camera)
    {
        camera.theta = -osc::fpi2;
        camera.phi = 0.0f;
    }

    void FocusAlongY(osc::PolarPerspectiveCamera& camera)
    {
        camera.theta = 0.0f;
        camera.phi = osc::fpi2;
    }

    void FocusAlongMinusY(osc::PolarPerspectiveCamera& camera)
    {
        camera.theta = 0.0f;
        camera.phi = -osc::fpi2;
    }

    void FocusAlongZ(osc::PolarPerspectiveCamera& camera)
    {
        camera.theta = 0.0f;
        camera.phi = 0.0f;
    }

    void FocusAlongMinusZ(osc::PolarPerspectiveCamera& camera)
    {
        camera.theta = osc::fpi;
        camera.phi = 0.0f;
    }

    void ZoomIn(osc::PolarPerspectiveCamera& camera)
    {
        camera.radius *= 0.8f;
    }

    void ZoomOut(osc::PolarPerspectiveCamera& camera)
    {
        camera.radius *= 1.2f;
    }

    void Reset(osc::PolarPerspectiveCamera& camera)
    {
        camera = {};
        camera.theta = osc::fpi4;
        camera.phi = osc::fpi4;
    }

    osc::PolarPerspectiveCamera CreateDefaultCamera()
    {
        osc::PolarPerspectiveCamera rv;
        rv.radius = 5.0f;
        return rv;
    }

    void AutoFocusCamera(osc::PolarPerspectiveCamera& camera, osc::BVH const& sceneBVH)
    {
        if (!sceneBVH.nodes.empty())
        {
            auto const& bvhRoot = sceneBVH.nodes[0].bounds;
            camera.focusPoint = -Midpoint(bvhRoot);
            camera.radius = 2.0f * LongestDim(bvhRoot);
            camera.theta = osc::fpi4;
            camera.phi = osc::fpi4;
        }
    }
}


// OpenSim utils
namespace
{
    float ComputeRimColor(OpenSim::Component const* selected,
        OpenSim::Component const* hovered,
        OpenSim::Component const* c)
    {
        while (c)
        {
            if (c == selected)
            {
                return 0.9f;
            }
            if (c == hovered)
            {
                return 0.4f;
            }
            if (!c->hasOwner())
            {
                return 0.0f;
            }
            c = &c->getOwner();
        }

        return 0.0f;
    }

    bool IsInclusiveChildOf(OpenSim::Component const* parent, OpenSim::Component const* c)
    {
        if (!c)
        {
            return false;
        }

        if (!parent)
        {
            return false;
        }

        for (;;)
        {
            if (c == parent)
            {
                return true;
            }

            if (!c->hasOwner())
            {
                return false;
            }

            c = &c->getOwner();
        }
    }
}


// rendering utils
namespace
{
    // helper method for making a render buffer (used in Render_target)
    gl::RenderBuffer makeMultisampledRenderBuffer(int samples, GLenum format, int w, int h)
    {
        gl::RenderBuffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, w, h);
        return rv;
    }

    gl::RenderBuffer makeRenderBuffer(GLenum format, int w, int h)
    {
        gl::RenderBuffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorage(GL_RENDERBUFFER, format, w, h);
        return rv;
    }

    // buffers used to render the scene
    struct RenderBuffers final {
        glm::ivec2 dims;
        int samples;

        // scene is MSXAAed + blended color buffer
        gl::RenderBuffer sceneRBO;
        gl::RenderBuffer sceneDepth24StencilRBO;
        gl::FrameBuffer sceneFBO;

        // rims are single-sampled, single-color, no blending
        gl::Texture2D rims2DTex;
        gl::RenderBuffer rims2DDepth24Stencil8RBO;
        gl::FrameBuffer rimsFBO;

        // output of the renderer
        gl::Texture2D outputTex;
        gl::Texture2D outputDepth24Stencil8Tex;
        gl::FrameBuffer outputFbo;

        RenderBuffers(glm::ivec2 dims_, int samples_) :
            dims{dims_},
            samples{samples_},

            sceneRBO{makeMultisampledRenderBuffer(samples, GL_RGBA, dims.x, dims.y)},
            sceneDepth24StencilRBO{makeMultisampledRenderBuffer(samples, GL_DEPTH24_STENCIL8, dims.x, dims.y)},
            sceneFBO{[this]() {
                gl::FrameBuffer rv;
                gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sceneRBO);
                gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, sceneDepth24StencilRBO);
                gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
                return rv;
            }()},

            rims2DTex{[this]()
            {
                gl::Texture2D rv;
                gl::BindTexture(rv);
                gl::TexImage2D(rv.type, 0, GL_RED, dims.x, dims.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
                gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
                gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
                gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                return rv;
            }()},
            rims2DDepth24Stencil8RBO{makeRenderBuffer(GL_DEPTH24_STENCIL8, dims.x, dims.y)},
            rimsFBO{[this]()
            {
                gl::FrameBuffer rv;
                gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rims2DTex, 0);
                gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, rims2DDepth24Stencil8RBO);
                gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
                return rv;
            }()},

            outputTex{[this]()
            {
               gl::Texture2D rv;
               gl::BindTexture(rv);
               gl::TexImage2D(rv.type, 0, GL_RGBA, dims.x, dims.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
               gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
               gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
               return rv;
           }()},
           outputDepth24Stencil8Tex{[this]()
            {
               gl::Texture2D rv;
               gl::BindTexture(rv);
               // https://stackoverflow.com/questions/27535727/opengl-create-a-depth-stencil-texture-for-reading
               gl::TexImage2D(rv.type, 0, GL_DEPTH24_STENCIL8, dims.x, dims.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
               return rv;
           }()},
           outputFbo{[this]()
           {
               gl::FrameBuffer rv;
               gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
               gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, outputTex, 0);
               gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, outputDepth24Stencil8Tex, 0);
               gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
               return rv;
           }()}
        {
            // ctor
        }

        void setDims(glm::ivec2 newDims)
        {
            if (newDims != dims)
            {
                *this = RenderBuffers{newDims, samples};
            }
        }

        void setSamples(int newSamples)
        {
            if (samples != newSamples)
            {
                *this = RenderBuffers{dims, newSamples};
            }
        }
    };

    struct SceneGPUInstanceData final {
        glm::mat4x3 modelMtx;
        glm::mat3 normalMtx;
        glm::vec4 rgba;
        float rimIntensity;
        int decorationIdx;
    };

    glm::mat4x3 GenerateFloorModelMatrix(glm::vec3 floorLocation, float fixupScaleFactor)
    {
        // rotate from XY (+Z dir) to ZY (+Y dir)
        glm::mat4 rv = glm::rotate(glm::mat4{1.0f}, -osc::fpi2, {1.0f, 0.0f, 0.0f});

        // make floor extend far in all directions
        rv = glm::scale(glm::mat4{1.0f}, {fixupScaleFactor * 100.0f, 1.0f, fixupScaleFactor * 100.0f}) * rv;

        rv = glm::translate(glm::mat4{1.0f}, fixupScaleFactor * floorLocation) * rv;

        return glm::mat4x3{rv};
    }

    void BindToInstanceAttributes(size_t offset)
    {
        gl::AttributeMat4x3 mmtxAttr{osc::SHADER_LOC_MATRIX_MODEL};
        gl::VertexAttribPointer(mmtxAttr, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*offset + offsetof(SceneGPUInstanceData, modelMtx));
        gl::VertexAttribDivisor(mmtxAttr, 1);
        gl::EnableVertexAttribArray(mmtxAttr);

        gl::AttributeMat3 normMtxAttr{osc::SHADER_LOC_MATRIX_NORMAL};
        gl::VertexAttribPointer(normMtxAttr, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*offset + offsetof(SceneGPUInstanceData, normalMtx));
        gl::VertexAttribDivisor(normMtxAttr, 1);
        gl::EnableVertexAttribArray(normMtxAttr);

        gl::AttributeVec4 colorAttr{osc::SHADER_LOC_COLOR_DIFFUSE};
        gl::VertexAttribPointer(colorAttr, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*offset + offsetof(SceneGPUInstanceData, rgba));
        gl::VertexAttribDivisor(colorAttr, 1);
        gl::EnableVertexAttribArray(colorAttr);

        gl::AttributeFloat rimAttr{osc::SHADER_LOC_COLOR_RIM};
        gl::VertexAttribPointer(rimAttr, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*offset + offsetof(SceneGPUInstanceData, rimIntensity));
        gl::VertexAttribDivisor(rimAttr, 1);
        gl::EnableVertexAttribArray(rimAttr);
    }

    // assumes `pos` is in-bounds
    void DrawBVHRecursive(
        osc::Mesh& cube,
        gl::UniformMat4& mtxUniform,
        osc::BVH const& bvh,
        int pos)
    {
        osc::BVHNode const& n = bvh.nodes[pos];

        glm::vec3 halfWidths = Dimensions(n.bounds) / 2.0f;
        glm::vec3 center = Midpoint(n.bounds);

        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
        glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
        glm::mat4 mmtx = mover * scaler;
        gl::Uniform(mtxUniform, mmtx);
        cube.Draw();

        if (n.nlhs >= 0)
        {
            // it's an internal node
            DrawBVHRecursive(cube, mtxUniform, bvh, pos+1);
            DrawBVHRecursive(cube, mtxUniform, bvh, pos+n.nlhs+1);
        }
    }

    void DrawBVH(
        osc::BVH const& sceneBVH,
        glm::mat4 const& viewMtx,
        glm::mat4 const& projMtx)
    {
        if (sceneBVH.nodes.empty())
        {
            return;
        }

        auto& shader = osc::App::shader<osc::SolidColorShader>();

        // common stuff
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjection, projMtx);
        gl::Uniform(shader.uView, viewMtx);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

        auto cube = osc::App::meshes().getCubeWireMesh();
        gl::BindVertexArray(cube->GetVertexArray());
        DrawBVHRecursive(*cube, shader.uModel, sceneBVH, 0);
        gl::BindVertexArray();
    }

    void DrawAABBs(
        nonstd::span<osc::ComponentDecoration const> decs,
        glm::mat4 const& viewMtx,
        glm::mat4 const& projMtx)
    {
        auto& shader = osc::App::shader<osc::SolidColorShader>();

        // common stuff
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjection, projMtx);
        gl::Uniform(shader.uView, viewMtx);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

        auto cube = osc::App::meshes().getCubeWireMesh();
        gl::BindVertexArray(cube->GetVertexArray());

        for (auto const& se : decs)
        {
            osc::AABB worldspaceAABB = GetWorldspaceAABB(se);
            glm::vec3 halfWidths = Dimensions(worldspaceAABB) / 2.0f;
            glm::vec3 center = Midpoint(worldspaceAABB);

            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
            glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
            glm::mat4 mmtx = mover * scaler;

            gl::Uniform(shader.uModel, mmtx);
            cube->Draw();
        }

        gl::BindVertexArray();
    }

    void DrawXZFloorLines(glm::mat4 const& viewMtx, glm::mat4 const& projMtx)
    {
        auto& shader = osc::App::shader<osc::SolidColorShader>();

        // common stuff
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjection, viewMtx);
        gl::Uniform(shader.uView, projMtx);

        auto yline = osc::App::meshes().getYLineMesh();
        gl::BindVertexArray(yline->GetVertexArray());

        // X
        gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, osc::fpi2, {0.0f, 0.0f, 1.0f}));
        gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
        yline->Draw();

        // Z
        gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, osc::fpi2, {1.0f, 0.0f, 0.0f}));
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
        yline->Draw();

        gl::BindVertexArray();
    }

    void DrawGrid(glm::mat4 const& rotationMatrix, glm::mat4 const& viewMtx, glm::mat4 const& projMtx)
    {
        auto& shader = osc::App::shader<osc::SolidColorShader>();

        gl::UseProgram(shader.program);
        gl::Uniform(shader.uModel, glm::scale(rotationMatrix, {50.0f, 50.0f, 1.0f}));
        gl::Uniform(shader.uView, viewMtx);
        gl::Uniform(shader.uProjection, projMtx);
        gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
        auto grid = osc::App::meshes().get100x100GridMesh();
        gl::BindVertexArray(grid->GetVertexArray());
        grid->Draw();
        gl::BindVertexArray();
    }

    void DrawXZGrid(glm::mat4 const& viewMtx, glm::mat4 const& projMtx)
    {
        DrawGrid(glm::rotate(glm::mat4{1.0f}, osc::fpi2, {1.0f, 0.0f, 0.0f}), viewMtx, projMtx);
    }

    void DrawXYGrid(glm::mat4 const& viewMtx, glm::mat4 const& projMtx)
    {
        DrawGrid(glm::mat4{1.0f}, viewMtx, projMtx);
    }

    void DrawYZGrid(glm::mat4 const& viewMtx, glm::mat4 const& projMtx)
    {
        DrawGrid(glm::rotate(glm::mat4{1.0f}, osc::fpi2, {0.0f, 1.0f, 0.0f}), viewMtx, projMtx);
    }

    struct ImGuiImageDetails final {
        osc::Rect rect = {};
        bool isHovered = false;
        bool isLeftClicked = false;
        bool isRightClicked = false;
    };

    ImGuiImageDetails drawTextureAsImGuiImageAndHittest(gl::Texture2D& tex, glm::vec2 dims)
    {
        osc::DrawTextureAsImGuiImage(tex, dims);

        ImGuiImageDetails rv;
        rv.rect.p1 = ImGui::GetItemRectMin();
        rv.rect.p2 = ImGui::GetItemRectMax();
        rv.isHovered = ImGui::IsItemHovered();
        rv.isLeftClicked = ImGui::IsItemHovered() && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
        rv.isRightClicked = ImGui::IsItemHovered() && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right);
        return rv;
    }

    // populates a high-level drawlist for an OpenSim model scene
    class CachedSceneDrawlist final {
    public:
        osc::UID getVersion() const
        {
            return m_Version;
        }

        nonstd::span<osc::ComponentDecoration const> get() const
        {
            return m_Decorations;
        }

        nonstd::span<osc::ComponentDecoration const> populate(
            osc::VirtualConstModelStatePair const& msp,
            osc::CustomDecorationOptions const& decorationOptions)
        {
            OpenSim::Component const* const selected = msp.getSelected();
            OpenSim::Component const* const hovered = msp.getHovered();
            OpenSim::Component const* const isolated = msp.getIsolated();

            if (msp.getModelVersion() != m_LastModelVersion ||
                msp.getStateVersion() != m_LastStateVersion ||
                selected != osc::FindComponent(msp.getModel(), m_LastSelection) ||
                hovered != osc::FindComponent(msp.getModel(), m_LastHover) ||
                isolated != osc::FindComponent(msp.getModel(), m_LastIsolation) ||
                msp.getFixupScaleFactor() != m_LastFixupFactor ||
                decorationOptions != m_LastDecorationOptions)
            {
                // update cache checks
                m_LastModelVersion = msp.getModelVersion();
                m_LastStateVersion = msp.getStateVersion();
                m_LastSelection = selected ? selected->getAbsolutePath() : OpenSim::ComponentPath{};
                m_LastHover = hovered ? hovered->getAbsolutePath() : OpenSim::ComponentPath{};
                m_LastIsolation = isolated ? isolated->getAbsolutePath() : OpenSim::ComponentPath{};
                m_LastFixupFactor = msp.getFixupScaleFactor();
                m_LastDecorationOptions = decorationOptions;
                m_Version = osc::UID{};

                // generate decorations from OpenSim/SimTK backend
                m_Decorations.clear();
                OSC_PERF("generate decorations");
                osc::GenerateModelDecorations(msp, m_Decorations, decorationOptions);
            }
            return m_Decorations;
        }

    private:
        osc::UID m_LastModelVersion;
        osc::UID m_LastStateVersion;
        OpenSim::ComponentPath m_LastSelection;
        OpenSim::ComponentPath m_LastHover;
        OpenSim::ComponentPath m_LastIsolation;
        float m_LastFixupFactor;
        osc::CustomDecorationOptions m_LastDecorationOptions;

        osc::UID m_Version;
        std::vector<osc::ComponentDecoration> m_Decorations;
    };

    class CachedBVH final {
    public:
        osc::UID getVersion() const
        {
            return m_Version;
        }

        osc::BVH const& get() const
        {
            return m_BVH;
        }

        osc::BVH const& populate(CachedSceneDrawlist const& drawlist)
        {
            if (drawlist.getVersion() == m_LastDrawlistVersion)
            {
                return m_BVH;
            }

            // update cache checks
            m_LastDrawlistVersion = drawlist.getVersion();
            m_Version = osc::UID{};

            OSC_PERF("generate BVH");
            osc::UpdateSceneBVH(drawlist.get(), m_BVH);
            return m_BVH;
        }
    private:
        osc::UID m_LastDrawlistVersion;
        osc::UID m_Version;
        osc::BVH m_BVH;
    };

    class CachedGPUDrawlist final {
    public:
        osc::UID getVersion() const
        {
            return m_Version;
        }

        nonstd::span<SceneGPUInstanceData const> get() const
        {
            return m_DrawListBuffer;
        }

        nonstd::span<SceneGPUInstanceData> populate(osc::VirtualConstModelStatePair const& msp, CachedSceneDrawlist const& drawlist)
        {
            if (drawlist.getVersion() == m_LastDrawlistVersion)
            {
                return m_DrawListBuffer;
            }

            // update cache checks
            m_LastDrawlistVersion = drawlist.getVersion();
            m_Version = osc::UID{};

            OpenSim::Component const* const selected = msp.getSelected();
            OpenSim::Component const* const hovered = msp.getHovered();
            OpenSim::Component const* const isolated = msp.getIsolated();

            auto decorations = drawlist.get();
            m_DrawListBuffer.clear();
            m_DrawListBuffer.reserve(decorations.size());

            // populate the list with the scene
            for (size_t i = 0; i < decorations.size(); ++i)
            {
                osc::ComponentDecoration const& se = decorations[i];

                if (isolated && !IsInclusiveChildOf(isolated, se.component))
                {
                    continue;  // skip rendering this (it's not in the isolated component)
                }

                SceneGPUInstanceData& ins = m_DrawListBuffer.emplace_back();
                ins.modelMtx = osc::ToMat4(se.transform);
                ins.normalMtx = osc::ToNormalMatrix(se.transform);
                ins.rgba = se.color;
                ins.rimIntensity = ComputeRimColor(selected, hovered, se.component);
                ins.decorationIdx = static_cast<int>(i);
            }

            return m_DrawListBuffer;
        }
    private:
        osc::UID m_LastDrawlistVersion;
        osc::UID m_Version;
        std::vector<SceneGPUInstanceData> m_DrawListBuffer;
    };

    struct ModelStateRenderParams final {
        glm::ivec2 Dimensions = {1, 1};
        int Samples = 1;
        bool WireframeMode = false;
        bool DrawMeshNormals = false;
        bool DrawRims = true;
        bool DrawFloor = true;
        glm::mat4 ViewMatrix{1.0f};
        glm::mat4 ProjectionMatrix{1.0f};
        glm::vec3 ViewPos = {0.0f, 0.0f, 0.0f};
        glm::vec3 LightDirection = {-0.34f, -0.25f, 0.05f};
        glm::vec3 LightColor = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
        glm::vec4 BackgroundColor = {0.89f, 0.89f, 0.89f, 1.0f};
        glm::vec4 RimColor = {1.0f, 0.4f, 0.0f, 0.85f};
        glm::vec3 FloorLocation = {0.0f, -0.001f, 0.0f};
        float FixupScaleFactor = 1.0f;
    };

    bool operator==(ModelStateRenderParams const& a, ModelStateRenderParams const& b)
    {
        return
            a.Dimensions == b.Dimensions &&
            a.Samples == b.Samples &&
            a.WireframeMode == b.WireframeMode &&
            a.DrawMeshNormals == b.DrawMeshNormals &&
            a.DrawRims == b.DrawRims &&
            a.DrawFloor == b.DrawFloor &&
            a.ViewMatrix == b.ViewMatrix &&
            a.ProjectionMatrix == b.ProjectionMatrix &&
            a.ViewPos == b.ViewPos &&
            a.LightDirection == b.LightDirection &&
            a.LightColor == b.LightColor &&
            a.BackgroundColor == b.BackgroundColor &&
            a.RimColor == b.RimColor &&
            a.FloorLocation == b.FloorLocation &&
            a.FixupScaleFactor == b.FixupScaleFactor;
    }

    class ModelStateRenderer final {
    public:

        glm::ivec2 getDimensions() const
        {
            return m_RenderTarget.dims;
        }

        int getSamples() const
        {
            return m_RenderTarget.samples;
        }

        void draw(ModelStateRenderParams const& params, osc::VirtualConstModelStatePair const& msp, CachedSceneDrawlist const& drawlist)
        {
            if (params == m_LastParams &&
                drawlist.getVersion() == m_LastDrawlistVersion)
            {
                return;  // nothing changed since the last render
            }

            // update cache checks
            m_LastParams = params;
            m_LastDrawlistVersion = drawlist.getVersion();

            // ensure underlying render target matches latest params
            m_RenderTarget.setDims(params.Dimensions);
            m_RenderTarget.setSamples(params.Samples);

            // populate GPU-friendly representation of the (higher-level) drawlist
            m_GPUDrawlist.populate(msp, drawlist);

            // upload data to the GPU
            gl::ArrayBuffer<SceneGPUInstanceData> instanceBuf{m_GPUDrawlist.get()};

            // setup top-level OpenGL state
            gl::Viewport(0, 0, m_RenderTarget.dims.x, m_RenderTarget.dims.y);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            gl::Enable(GL_BLEND);
            gl::Enable(GL_DEPTH_TEST);
            gl::Disable(GL_SCISSOR_TEST);

            // draw scene
            {
                gl::BindFramebuffer(GL_FRAMEBUFFER, m_RenderTarget.sceneFBO);
                gl::ClearColor(params.BackgroundColor);
                gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                if (params.WireframeMode)
                {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                }

                // draw the floor first, so that the transparent geometry in the scene also shows
                // the floor texture
                if (params.DrawFloor & osc::UiModelViewerFlags_DrawFloor)
                {
                    auto& basicShader = osc::App::shader<osc::GouraudShader>();

                    gl::UseProgram(basicShader.program);
                    gl::Uniform(basicShader.uProjMat, params.ProjectionMatrix);
                    gl::Uniform(basicShader.uViewMat, params.ViewMatrix);
                    glm::mat4 mtx = GenerateFloorModelMatrix(params.FloorLocation, params.FixupScaleFactor);
                    gl::Uniform(basicShader.uModelMat, mtx);
                    gl::Uniform(basicShader.uNormalMat, osc::ToNormalMatrix(mtx));
                    gl::Uniform(basicShader.uLightDir, params.LightDirection);
                    gl::Uniform(basicShader.uLightColor, params.LightColor);
                    gl::Uniform(basicShader.uViewPos, params.ViewPos);
                    gl::Uniform(basicShader.uDiffuseColor, {1.0f, 1.0f, 1.0f, 1.0f});
                    gl::Uniform(basicShader.uIsTextured, true);
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(m_ChequerTexture);
                    gl::Uniform(basicShader.uSampler0, gl::textureIndex<GL_TEXTURE0>());
                    auto floor = osc::App::meshes().getFloorMesh();
                    gl::BindVertexArray(floor->GetVertexArray());
                    floor->Draw();
                    gl::BindVertexArray();
                }

                auto& instancedShader = osc::App::shader<osc::InstancedGouraudColorShader>();
                gl::UseProgram(instancedShader.program);
                gl::Uniform(instancedShader.uProjMat, params.ProjectionMatrix);
                gl::Uniform(instancedShader.uViewMat, params.ViewMatrix);
                gl::Uniform(instancedShader.uLightDir, params.LightDirection);
                gl::Uniform(instancedShader.uLightColor, params.LightColor);
                gl::Uniform(instancedShader.uViewPos, params.ViewPos);

                nonstd::span<SceneGPUInstanceData const> instances = m_GPUDrawlist.get();
                nonstd::span<osc::ComponentDecoration const> decs = drawlist.get();

                size_t pos = 0;
                size_t ninstances = instances.size();

                while (pos < ninstances)
                {
                    osc::ComponentDecoration const& se = decs[instances[pos].decorationIdx];

                    // batch
                    size_t end = pos + 1;
                    while (end < ninstances &&
                        decs[instances[end].decorationIdx].mesh == se.mesh)
                    {
                        ++end;
                    }

                    // if the last element in a batch is opaque, then all the preceding ones should be
                    // also and we can skip blend-testing the entire batch
                    if (instances[end-1].rgba.a >= 0.99f)
                    {
                        gl::Disable(GL_BLEND);
                    }
                    else
                    {
                        gl::Enable(GL_BLEND);
                    }

                    gl::BindVertexArray(se.mesh->GetVertexArray());
                    gl::BindBuffer(instanceBuf);
                    BindToInstanceAttributes(pos);
                    se.mesh->DrawInstanced(end-pos);
                    gl::BindVertexArray();

                    pos = end;
                }

                if (params.WireframeMode)
                {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
            }

            // draw mesh normals, if requested
            if (params.DrawMeshNormals)
            {
                auto& normalShader = osc::App::shader<osc::NormalsShader>();
                gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
                gl::UseProgram(normalShader.program);
                gl::Uniform(normalShader.uProjMat, params.ProjectionMatrix);
                gl::Uniform(normalShader.uViewMat, params.ViewMatrix);

                nonstd::span<SceneGPUInstanceData const> instances = m_GPUDrawlist.get();
                nonstd::span<osc::ComponentDecoration const> decs = drawlist.get();

                for (SceneGPUInstanceData const& inst : instances)
                {
                    osc::ComponentDecoration const& se = decs[inst.decorationIdx];

                    gl::Uniform(normalShader.uModelMat, inst.modelMtx);
                    gl::Uniform(normalShader.uNormalMat, inst.normalMtx);
                    gl::BindVertexArray(se.mesh->GetVertexArray());
                    se.mesh->Draw();
                }
                gl::BindVertexArray();
            }

            // blit scene render to non-MSXAAed output texture
            gl::BindFramebuffer(GL_READ_FRAMEBUFFER, m_RenderTarget.sceneFBO);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, m_RenderTarget.outputFbo);
            gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
            gl::BlitFramebuffer(0, 0, m_RenderTarget.dims.x, m_RenderTarget.dims.y,
                0, 0, m_RenderTarget.dims.x, m_RenderTarget.dims.y,
                GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

            // draw rims
            if (params.DrawRims)
            {
                gl::BindFramebuffer(GL_FRAMEBUFFER, m_RenderTarget.rimsFBO);
                gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                auto& iscs = osc::App::shader<osc::InstancedSolidColorShader>();
                gl::UseProgram(iscs.program);
                gl::Uniform(iscs.uVP, params.ProjectionMatrix * params.ViewMatrix);

                nonstd::span<SceneGPUInstanceData const> instances = m_GPUDrawlist.get();
                nonstd::span<osc::ComponentDecoration const> decs = drawlist.get();

                size_t pos = 0;
                size_t ninstances = instances.size();

                // drawcalls & figure out rim AABB
                osc::AABB rimAABB;
                rimAABB.min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
                rimAABB.max = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
                bool hasRims = false;

                while (pos < ninstances)
                {
                    SceneGPUInstanceData const& inst = instances[pos];
                    osc::ComponentDecoration const& se = decs[inst.decorationIdx];

                    // batch
                    size_t end = pos + 1;
                    while (end < ninstances &&
                        decs[instances[end].decorationIdx].mesh == se.mesh &&
                        instances[end].rimIntensity == inst.rimIntensity)
                    {
                        ++end;
                    }

                    if (inst.rimIntensity < 0.001f)
                    {
                        pos = end;
                        continue;  // skip rendering rimless elements
                    }

                    hasRims = true;

                    // union the rims for scissor testing later
                    for (size_t i = pos; i < end; ++i)
                    {
                        rimAABB = Union(rimAABB, GetWorldspaceAABB(decs[instances[i].decorationIdx]));
                    }

                    gl::Uniform(iscs.uColor, {inst.rimIntensity, 0.0f, 0.0f, 1.0f});
                    gl::BindVertexArray(se.mesh->GetVertexArray());
                    gl::BindBuffer(instanceBuf);
                    BindToInstanceAttributes(pos);
                    se.mesh->DrawInstanced(end-pos);
                    gl::BindVertexArray();

                    pos = end;
                }

                if (hasRims)
                {
                    glm::vec2 rimThickness = glm::vec2{1.5f} / glm::vec2{m_RenderTarget.dims};

                    // care: edge-detection kernels are expensive
                    //
                    // calculate a screenspace bounding box that surrounds the rims so that the
                    // edge detection shader only had to run on a smaller subset of the screen
                    osc::AABB screenspaceRimBounds = TransformAABB(rimAABB, params.ProjectionMatrix * params.ViewMatrix);
                    auto verts = ToCubeVerts(screenspaceRimBounds);

                    osc::Rect screenBounds{verts[0], verts[0]};

                    for (size_t i = 1; i < verts.size(); ++i)
                    {
                        glm::vec2 p = verts[i];  // dump Z
                        screenBounds.p1 = osc::Min(p, screenBounds.p1);
                        screenBounds.p2 = osc::Max(p, screenBounds.p2);
                    }
                    screenBounds.p1 -= rimThickness;
                    screenBounds.p2 += rimThickness;

                    glm::ivec2 renderDims = m_RenderTarget.dims;
                    glm::ivec2 min{
                        static_cast<int>((screenBounds.p1.x + 1.0f)/2.0f * renderDims.x),
                        static_cast<int>((screenBounds.p1.y + 1.0f)/2.0f * renderDims.y)};
                    glm::ivec2 max{
                        static_cast<int>((screenBounds.p2.x + 1.0f)/2.0f * renderDims.x),
                        static_cast<int>((screenBounds.p2.y + 1.0f)/2.0f * renderDims.y)};

                    int x = std::max(0, min.x);
                    int y = std::max(0, min.y);
                    int w = max.x - min.x;
                    int h = max.y - min.y;

                    // rims FBO now contains *solid* colors that need to be edge-detected

                    // write rims over the output output
                    gl::BindFramebuffer(GL_FRAMEBUFFER, m_RenderTarget.outputFbo);

                    auto& edgeDetectShader = osc::App::shader<osc::EdgeDetectionShader>();
                    gl::UseProgram(edgeDetectShader.program);
                    gl::Uniform(edgeDetectShader.uMVP, gl::identity);
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(m_RenderTarget.rims2DTex);
                    gl::Uniform(edgeDetectShader.uSampler0, gl::textureIndex<GL_TEXTURE0>());
                    gl::Uniform(edgeDetectShader.uRimRgba, {0.95f, 0.40f, 0.0f, 0.70f});
                    gl::Uniform(edgeDetectShader.uRimThickness, rimThickness);
                    gl::Enable(GL_SCISSOR_TEST);
                    glScissor(x, y, w, h);
                    gl::Enable(GL_BLEND);
                    gl::Disable(GL_DEPTH_TEST);
                    auto quad = osc::App::meshes().getTexturedQuadMesh();
                    gl::BindVertexArray(quad->GetVertexArray());
                    quad->Draw();
                    gl::BindVertexArray();
                    gl::Enable(GL_DEPTH_TEST);
                    gl::Disable(GL_SCISSOR_TEST);
                }
            }

            gl::Enable(GL_BLEND);
            gl::Enable(GL_DEPTH_TEST);
            gl::Disable(GL_SCISSOR_TEST);
            gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
        }

        gl::Texture2D& updOutputTexture()
        {
            return m_RenderTarget.outputTex;
        }

        gl::FrameBuffer& updOutputFBO()
        {
            return m_RenderTarget.outputFbo;
        }

    private:
        // caching
        ModelStateRenderParams m_LastParams;
        osc::UID m_LastDrawlistVersion;

        CachedGPUDrawlist m_GPUDrawlist;
        gl::Texture2D m_ChequerTexture = osc::genChequeredFloorTexture();
        RenderBuffers m_RenderTarget{{1, 1}, 1};
    };
}


// GUI ruler
namespace
{
    // state associated with a 3D ruler that users can use to measure things
    // in the scene
    class GuiRuler final {
        enum class State { Inactive, WaitingForFirstPoint, WaitingForSecondPoint };
        State m_State = State::Inactive;
        glm::vec3 m_StartWorldPos = {0.0f, 0.0f, 0.0f};

    public:
        void draw(std::pair<OpenSim::Component const*, glm::vec3> const& htResult,
            osc::PolarPerspectiveCamera const& sceneCamera,
            osc::Rect renderRect)
        {
            if (m_State == State::Inactive)
            {
                return;
            }

            // users can exit measuring through these actions
            if (ImGui::IsKeyDown(SDL_SCANCODE_ESCAPE) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                StopMeasuring();
                return;
            }

            // users can "finish" the measurement through these actions
            if (m_State == State::WaitingForSecondPoint && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                StopMeasuring();
                return;
            }

            glm::vec2 mouseLoc = ImGui::GetMousePos();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImU32 circleMousedOverNothingColor = ImGui::ColorConvertFloat4ToU32({1.0f, 0.0f, 0.0f, 0.6f});
            ImU32 circleColor = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 0.8f});
            ImU32 lineColor = circleColor;
            ImU32 textBgColor = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 1.0f});
            ImU32 textColor = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            float circleRadius = 5.0f;
            float lineThickness = 3.0f;
            glm::vec2 labelOffsetWhenNoLine = {10.0f, -10.0f};

            auto drawTooltipWithBg = [&dl, &textBgColor, &textColor](glm::vec2 const& pos, char const* text)
            {
                glm::vec2 sz = ImGui::CalcTextSize(text);
                float bgPad = 5.0f;
                float edgeRounding = bgPad - 2.0f;

                dl->AddRectFilled(pos - bgPad, pos + sz + bgPad, textBgColor, edgeRounding);
                dl->AddText(pos, textColor, text);
            };

            if (m_State == State::WaitingForFirstPoint)
            {
                if (!htResult.first)
                {
                    // not mousing over anything
                    dl->AddCircleFilled(mouseLoc, circleRadius, circleMousedOverNothingColor);
                    return;
                }
                else
                {
                    // mousing over something
                    dl->AddCircleFilled(mouseLoc, circleRadius, circleColor);
                    char buf[1024];
                    std::snprintf(buf, sizeof(buf), "%s @ (%.2f, %.2f, %.2f)", htResult.first->getName().c_str(), htResult.second.x, htResult.second.y, htResult.second.z);
                    drawTooltipWithBg(mouseLoc + labelOffsetWhenNoLine, buf);

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        m_State = State::WaitingForSecondPoint;
                        m_StartWorldPos = htResult.second;
                    }
                    return;
                }
            }
            else if (m_State == State::WaitingForSecondPoint)
            {
                glm::vec2 startScreenPos = sceneCamera.projectOntoScreenRect(m_StartWorldPos, renderRect);

                if (htResult.first)
                {
                    // user is moused over something, so draw a line + circle between the two hitlocs
                    glm::vec2 endScreenPos = mouseLoc;
                    glm::vec2 lineScreenDir = glm::normalize(startScreenPos - endScreenPos);
                    glm::vec2 offsetVec = 15.0f * glm::vec2{lineScreenDir.y, -lineScreenDir.x};
                    glm::vec2 lineMidpoint = (startScreenPos + endScreenPos) / 2.0f;
                    float lineWorldLen = glm::length(htResult.second - m_StartWorldPos);

                    dl->AddCircleFilled(startScreenPos, circleRadius, circleColor);
                    dl->AddLine(startScreenPos, endScreenPos, lineColor, lineThickness);
                    dl->AddCircleFilled(endScreenPos, circleRadius, circleColor);

                    // label the line's length
                    {
                        char buf[1024];
                        std::snprintf(buf, sizeof(buf), "%.5f", lineWorldLen);
                        drawTooltipWithBg(lineMidpoint + offsetVec, buf);
                    }

                    // label the endpoint's component + coord
                    {
                        char buf[1024];
                        std::snprintf(buf, sizeof(buf), "%s @ (%.2f, %.2f, %.2f)", htResult.first->getName().c_str(), htResult.second.x, htResult.second.y, htResult.second.z);
                        drawTooltipWithBg(mouseLoc + offsetVec, buf);
                    }
                }
                else
                {
                    dl->AddCircleFilled(startScreenPos, circleRadius, circleColor);
                }
            }
        }

        void StartMeasuring()
        {
            m_State = State::WaitingForFirstPoint;
        }

        void StopMeasuring()
        {
            m_State = State::Inactive;
        }

        bool IsMeasuring() const
        {
            return m_State != State::Inactive;
        }
    };
}


// private IMPL
class osc::UiModelViewer::Impl final {
public:
    explicit Impl(UiModelViewerFlags flags) :
        m_Flags{std::move(flags)}
    {
        m_Camera.theta = fpi4;
        m_Camera.phi = fpi4;
    }

    bool isLeftClicked() const
    {
        return m_RenderImage.isLeftClicked;
    }

    bool isRightClicked() const
    {
        return m_RenderImage.isRightClicked;
    }

    bool isMousedOver() const
    {
        return m_RenderImage.isHovered;
    }

    void requestAutoFocus()
    {
        m_AutoFocusCameraNextFrame = true;
    }

    osc::UiModelViewerResponse draw(VirtualConstModelStatePair const& rs)
    {
        UiModelViewerResponse rv;

        handleUserInput();
        drawMainMenu();

        if (!ImGui::BeginChild("##child", { 0.0f, 0.0f }, false, ImGuiWindowFlags_NoMove))
        {
            // render window isn't visible
            m_RenderImage = {};
            return rv;
        }

        recomputeSceneLightPosition();

        // populate render buffers
        m_SceneDrawlist.populate(rs, m_DecorationOptions);
        m_BVH.populate(m_SceneDrawlist);

        std::pair<OpenSim::Component const*, glm::vec3> htResult = hittestRenderWindow(rs);

        // auto-focus the camera, if the user requested it last frame
        //
        // care: indirectly depends on the scene drawlist being up-to-date
        if (m_AutoFocusCameraNextFrame)
        {
            AutoFocusCamera(m_Camera, m_BVH.get());
            m_AutoFocusCameraNextFrame = false;
        }

        // render into texture
        drawSceneTexture(rs);

        // also render in-scene overlays into the texture
        drawInSceneOverlays();

        // blit texture as an ImGui::Image
        m_RenderImage = drawTextureAsImGuiImageAndHittest(m_Rendererer.updOutputTexture(), ImGui::GetContentRegionAvail());

        // draw any ImGui-based overlays over the image
        drawImGuiOverlays();

        if (m_Ruler.IsMeasuring())
        {
            m_Ruler.draw(htResult, m_Camera, m_RenderImage.rect);
        }

        ImGui::EndChild();

        // handle return value

        if (!m_Ruler.IsMeasuring())
        {
            // only populate response if the ruler isn't blocking hittesting etc.
            rv.hovertestResult = htResult.first;
            rv.isMousedOver = m_RenderImage.isHovered;
            if (rv.isMousedOver)
            {
                rv.mouse3DLocation = htResult.second;
            }
        }

        return rv;
    }

private:

    void handleUserInput()
    {
        // update camera if necessary
        if (m_RenderImage.isHovered)
        {
            bool ctrlDown = osc::IsCtrlOrSuperDown();

            if (ImGui::IsKeyReleased(SDL_SCANCODE_X))
            {
                if (ctrlDown)
                {
                    FocusAlongMinusX(m_Camera);
                } else
                {
                    FocusAlongX(m_Camera);
                }
            }
            if (ImGui::IsKeyPressed(SDL_SCANCODE_Y))
            {
                if (!ctrlDown)
                {
                    FocusAlongY(m_Camera);
                }
            }
            if (ImGui::IsKeyPressed(SDL_SCANCODE_F))
            {
                if (ctrlDown)
                {
                    m_AutoFocusCameraNextFrame = true;
                }
                else
                {
                    Reset(m_Camera);
                }
            }
            if (ctrlDown && (ImGui::IsKeyPressed(SDL_SCANCODE_8)))
            {
                // solidworks keybind
                m_AutoFocusCameraNextFrame = true;
            }
            UpdatePolarCameraFromImGuiUserInput(Dimensions(m_RenderImage.rect), m_Camera);
        }
    }

    void drawMainMenu()
    {
        if (ImGui::BeginMenuBar())
        {
            drawMainMenuContent();
            ImGui::EndMenuBar();
        }
    }

    void drawMainMenuContent()
    {
        if (ImGui::BeginMenu("Options"))
        {
            drawOptionsMenuContent();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene"))
        {
            drawSceneMenuContent();
            ImGui::EndMenu();
        }

        drawRulerMeasurementToggleButton();
    }

    void drawRulerMeasurementToggleButton()
    {
        if (m_Ruler.IsMeasuring())
        {
            if (ImGui::MenuItem(ICON_FA_RULER " measuring", nullptr, false, false))
            {
                m_Ruler.StopMeasuring();
            }
        }
        else
        {
            if (ImGui::MenuItem(ICON_FA_RULER " measure", nullptr, false, true))
            {
                m_Ruler.StartMeasuring();
            }
            osc::DrawTooltipIfItemHovered("Measure distance", "EXPERIMENTAL: take a *rough* measurement of something in the scene - the UI for this needs to be improved, a lot ;)");
        }
    }

    void drawOptionsMenuContent()
    {
        drawMuscleDecorationsStyleComboBox();
        drawMuscleSizingStyleComboBox();
        drawMuscleColoringStyleComboBox();
        ImGui::Checkbox("wireframe mode", &m_RendererParams.WireframeMode);
        ImGui::Checkbox("show normals", &m_RendererParams.DrawMeshNormals);
        ImGui::Checkbox("draw rims", &m_RendererParams.DrawRims);
        ImGui::CheckboxFlags("show XZ grid", &m_Flags, osc::UiModelViewerFlags_DrawXZGrid);
        ImGui::CheckboxFlags("show XY grid", &m_Flags, osc::UiModelViewerFlags_DrawXYGrid);
        ImGui::CheckboxFlags("show YZ grid", &m_Flags, osc::UiModelViewerFlags_DrawYZGrid);
        ImGui::CheckboxFlags("show alignment axes", &m_Flags, osc::UiModelViewerFlags_DrawAlignmentAxes);
        ImGui::CheckboxFlags("show grid lines", &m_Flags, osc::UiModelViewerFlags_DrawAxisLines);
        ImGui::CheckboxFlags("show AABBs", &m_Flags, osc::UiModelViewerFlags_DrawAABBs);
        ImGui::CheckboxFlags("show BVH", &m_Flags, osc::UiModelViewerFlags_DrawBVH);
        ImGui::CheckboxFlags("show floor", &m_Flags, osc::UiModelViewerFlags_DrawFloor);
    }

    void drawMuscleDecorationsStyleComboBox()
    {
        osc::MuscleDecorationStyle s = m_DecorationOptions.getMuscleDecorationStyle();
        nonstd::span<osc::MuscleDecorationStyle const> allStyles = osc::GetAllMuscleDecorationStyles();
        nonstd::span<char const* const>  allNames = osc::GetAllMuscleDecorationStyleStrings();
        int i = osc::GetIndexOf(s);

        if (ImGui::Combo("muscle decoration style", &i, allNames.data(), static_cast<int>(allStyles.size())))
        {
            m_DecorationOptions.setMuscleDecorationStyle(allStyles[i]);
        }
    }

    void drawMuscleSizingStyleComboBox()
    {
        osc::MuscleSizingStyle s = m_DecorationOptions.getMuscleSizingStyle();
        nonstd::span<osc::MuscleSizingStyle const> allStyles = osc::GetAllMuscleSizingStyles();
        nonstd::span<char const* const>  allNames = osc::GetAllMuscleSizingStyleStrings();
        int i = osc::GetIndexOf(s);

        if (ImGui::Combo("muscle sizing style", &i, allNames.data(), static_cast<int>(allStyles.size())))
        {
            m_DecorationOptions.setMuscleSizingStyle(allStyles[i]);
        }
    }

    void drawMuscleColoringStyleComboBox()
    {
        osc::MuscleColoringStyle s = m_DecorationOptions.getMuscleColoringStyle();
        nonstd::span<osc::MuscleColoringStyle const> allStyles = osc::GetAllMuscleColoringStyles();
        nonstd::span<char const* const>  allNames = osc::GetAllMuscleColoringStyleStrings();
        int i = osc::GetIndexOf(s);

        if (ImGui::Combo("muscle coloring", &i, allNames.data(), static_cast<int>(allStyles.size())))
        {
            m_DecorationOptions.setMuscleColoringStyle(allStyles[i]);
        }
    }

    void drawSceneMenuContent()
    {
        ImGui::Text("reposition camera:");
        ImGui::Separator();

        auto makeHoverTooltip = [](char const* msg)
        {
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(msg);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        };

        if (ImGui::Button("+X"))
        {
            FocusAlongX(m_Camera);
        }
        makeHoverTooltip("Position camera along +X, pointing towards the center. Hotkey: X");
        ImGui::SameLine();
        if (ImGui::Button("-X"))
        {
            FocusAlongMinusX(m_Camera);
        }
        makeHoverTooltip("Position camera along -X, pointing towards the center. Hotkey: Ctrl+X");

        ImGui::SameLine();
        if (ImGui::Button("+Y"))
        {
            FocusAlongY(m_Camera);
        }
        makeHoverTooltip("Position camera along +Y, pointing towards the center. Hotkey: Y");
        ImGui::SameLine();
        if (ImGui::Button("-Y"))
        {
            FocusAlongMinusY(m_Camera);
        }
        makeHoverTooltip("Position camera along -Y, pointing towards the center. (no hotkey, because Ctrl+Y is taken by 'Redo'");

        ImGui::SameLine();
        if (ImGui::Button("+Z"))
        {
            FocusAlongZ(m_Camera);
        }
        makeHoverTooltip("Position camera along +Z, pointing towards the center. Hotkey: Z");
        ImGui::SameLine();
        if (ImGui::Button("-Z"))
        {
            FocusAlongMinusZ(m_Camera);
        }
        makeHoverTooltip("Position camera along -Z, pointing towards the center. (no hotkey, because Ctrl+Z is taken by 'Undo')");

        if (ImGui::Button("Zoom in"))
        {
            ZoomIn(m_Camera);
        }

        ImGui::SameLine();
        if (ImGui::Button("Zoom out"))
        {
            ZoomOut(m_Camera);
        }

        if (ImGui::Button("reset camera"))
        {
            Reset(m_Camera);
        }
        makeHoverTooltip("Reset the camera to its initial (default) location. Hotkey: F");

        if (ImGui::Button("Auto-focus camera"))
        {
            m_AutoFocusCameraNextFrame = true;
        }
        makeHoverTooltip("Try to automatically adjust the camera's zoom etc. to suit the model's dimensions. Hotkey: Ctrl+F");

        if (ImGui::Button("Export to .dae"))
        {
            tryExportSceneToDAE();

        }
        makeHoverTooltip("Try to export the 3D scene to a portable DAE file, so that it can be viewed in 3rd-party modelling software, such as Blender");

        ImGui::Dummy({0.0f, 10.0f});
        ImGui::Text("advanced camera properties:");
        ImGui::Separator();
        osc::SliderMetersFloat("radius", &m_Camera.radius, 0.0f, 10.0f);
        ImGui::SliderFloat("theta", &m_Camera.theta, 0.0f, 2.0f * osc::fpi);
        ImGui::SliderFloat("phi", &m_Camera.phi, 0.0f, 2.0f * osc::fpi);
        ImGui::InputFloat("fov", &m_Camera.fov);
        osc::InputMetersFloat("znear", &m_Camera.znear);
        osc::InputMetersFloat("zfar", &m_Camera.zfar);
        ImGui::NewLine();
        osc::SliderMetersFloat("pan_x", &m_Camera.focusPoint.x, -100.0f, 100.0f);
        osc::SliderMetersFloat("pan_y", &m_Camera.focusPoint.y, -100.0f, 100.0f);
        osc::SliderMetersFloat("pan_z", &m_Camera.focusPoint.z, -100.0f, 100.0f);

        ImGui::Dummy({0.0f, 10.0f});
        ImGui::Text("advanced scene properties:");
        ImGui::Separator();
        ImGui::ColorEdit3("light_color", glm::value_ptr(m_RendererParams.LightColor));
        ImGui::ColorEdit3("background color", glm::value_ptr(m_RendererParams.BackgroundColor));
        osc::InputMetersFloat3("floor location", glm::value_ptr(m_RendererParams.FloorLocation));
        makeHoverTooltip("Set the origin location of the scene's chequered floor. This is handy if you are working on smaller models, or models that need a floor somewhere else");
    }

    void recomputeSceneLightPosition()
    {
        // automatically change lighting position based on camera position
        glm::vec3 p = glm::normalize(-m_Camera.focusPoint - m_Camera.getPos());
        glm::vec3 up = {0.0f, 1.0f, 0.0f};
        glm::vec3 mp = glm::rotate(glm::mat4{1.0f}, 1.05f * fpi4, up) * glm::vec4{p, 0.0f};
        m_RendererParams.LightDirection = glm::normalize(mp + -up);
    }

    std::pair<OpenSim::Component const*, glm::vec3> hittestRenderWindow(osc::VirtualConstModelStatePair const& msp)
    {
        std::pair<OpenSim::Component const*, glm::vec3> rv = {nullptr, {0.0f, 0.0f, 0.0f}};

        if (!m_RenderImage.isHovered ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            // only do the hit test if the user isn't currently dragging their mouse around
            return rv;
        }

        // figure out mouse pos in panel's NDC system
        glm::vec2 windowScreenPos = ImGui::GetWindowPos();  // where current ImGui window is in the screen
        glm::vec2 mouseScreenPos = ImGui::GetMousePos();  // where mouse is in the screen
        glm::vec2 mouseWindowPos = mouseScreenPos - windowScreenPos;  // where mouse is in current window
        glm::vec2 cursorWindowPos = ImGui::GetCursorPos();  // where cursor is in current window
        glm::vec2 mouseItemPos = mouseWindowPos - cursorWindowPos;  // where mouse is in current item
        glm::vec2 itemDims = ImGui::GetContentRegionAvail();  // how big current window will be

        // un-project the mouse position as a ray in worldspace
        osc::Line cameraRay = m_Camera.unprojectTopLeftPosToWorldRay(mouseItemPos, itemDims);

        // use scene BVH to intersect that ray with the scene
        std::vector<BVHCollision> m_SceneHittestResults;
        BVH_GetRayAABBCollisions(m_BVH.get(), cameraRay, &m_SceneHittestResults);

        // go through triangle BVHes to figure out which, if any, triangle is closest intersecting
        int closestIdx = -1;
        float closestDistance = std::numeric_limits<float>::max();
        glm::vec3 closestWorldLoc = {0.0f, 0.0f, 0.0f};

        // iterate through each scene-level hit and perform a triangle-level hittest
        nonstd::span<osc::ComponentDecoration const> decs = m_SceneDrawlist.get();
        OpenSim::Component const* const isolated = msp.getIsolated();

        for (osc::BVHCollision const& c : m_SceneHittestResults)
        {
            int instanceIdx = c.primId;

            if (isolated && !IsInclusiveChildOf(isolated, decs[instanceIdx].component))
            {
                continue;  // it's not in the current isolation
            }

            glm::mat4 instanceMmtx = ToMat4(decs[instanceIdx].transform);
            osc::Line cameraRayModelspace = TransformLine(cameraRay, ToInverseMat4(decs[instanceIdx].transform));

            auto maybeCollision = decs[instanceIdx].mesh->getClosestRayTriangleCollisionModelspace(cameraRayModelspace);

            if (maybeCollision && maybeCollision.distance < closestDistance)
            {
                closestIdx = instanceIdx;
                closestDistance = maybeCollision.distance;
                glm::vec3 closestLocModelspace = cameraRayModelspace.origin + closestDistance*cameraRayModelspace.dir;
                closestWorldLoc = glm::vec3{instanceMmtx * glm::vec4{closestLocModelspace, 1.0f}};
            }
        }

        if (closestIdx >= 0)
        {
            rv.first = decs[closestIdx].component;
            rv.second = closestWorldLoc;
        }

        return rv;
    }

    void drawSceneTexture(osc::VirtualConstModelStatePair const& rs)
    {
        // setup render params
        ImVec2 contentRegion = ImGui::GetContentRegionAvail();
        if (contentRegion.x >= 1.0f && contentRegion.y >= 1.0f)
        {
            glm::ivec2 dims{static_cast<int>(contentRegion.x), static_cast<int>(contentRegion.y)};
            m_RendererParams.Dimensions = dims;
            m_RendererParams.Samples = osc::App::get().getMSXAASamplesRecommended();
        }

        m_RendererParams.DrawFloor = m_Flags & UiModelViewerFlags_DrawFloor;
        m_RendererParams.ViewMatrix = m_Camera.getViewMtx();
        m_RendererParams.ProjectionMatrix = m_Camera.getProjMtx(AspectRatio(m_Rendererer.getDimensions()));
        m_RendererParams.ViewPos = m_Camera.getPos();
        m_RendererParams.FixupScaleFactor = rs.getFixupScaleFactor();

        m_Rendererer.draw(m_RendererParams, rs, m_SceneDrawlist);
    }

    // draws overlays that are "in scene" - i.e. they are part of the rendered texture
    void drawInSceneOverlays()
    {
        glm::mat4 viewMtx = m_Camera.getViewMtx();
        glm::mat4 projMtx = m_Camera.getProjMtx(AspectRatio(m_Rendererer.getDimensions()));

        gl::BindFramebuffer(GL_FRAMEBUFFER, m_Rendererer.updOutputFBO());
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);

        if (m_Flags & osc::UiModelViewerFlags_DrawXZGrid)
        {
            DrawXZGrid(viewMtx, projMtx);
        }

        if (m_Flags & osc::UiModelViewerFlags_DrawXYGrid)
        {
            DrawXYGrid(viewMtx, projMtx);
        }

        if (m_Flags & osc::UiModelViewerFlags_DrawYZGrid)
        {
            DrawYZGrid(viewMtx, projMtx);
        }

        if (m_Flags & osc::UiModelViewerFlags_DrawAxisLines)
        {
            DrawXZFloorLines(viewMtx, projMtx);
        }

        if (m_Flags & osc::UiModelViewerFlags_DrawAABBs)
        {
            DrawAABBs(m_SceneDrawlist.get(), viewMtx, projMtx);
        }

        if (m_Flags & osc::UiModelViewerFlags_DrawBVH)
        {
            DrawBVH(m_BVH.get(), viewMtx, projMtx);
        }

        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
    }

    void drawImGuiOverlays()
    {
        if (m_Flags & osc::UiModelViewerFlags_DrawAlignmentAxes)
        {
            DrawAlignmentAxesOverlayInBottomRightOf(m_Camera.getViewMtx(), m_RenderImage.rect);
        }
    }

    void tryExportSceneToDAE()
    {
        std::filesystem::path p =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("dae");

        std::ofstream outfile{p};

        if (!outfile)
        {
            osc::log::error("cannot save to %s: IO error", p.string().c_str());
            return;
        }

        std::vector<osc::BasicSceneElement> basicDecs;
        for (osc::ComponentDecoration const& dec : m_SceneDrawlist.get())
        {
            osc::BasicSceneElement& basic = basicDecs.emplace_back();
            basic.transform = dec.transform;
            basic.mesh = dec.mesh;
            basic.color = dec.color;
        }

        osc::WriteDecorationsAsDAE(basicDecs, outfile);
        osc::log::info("wrote scene as a DAE file to %s", p.string().c_str());
    }

    // widget state
    UiModelViewerFlags m_Flags = UiModelViewerFlags_Default;
    osc::CustomDecorationOptions m_DecorationOptions;

    // scene state
    CachedSceneDrawlist m_SceneDrawlist;
    CachedBVH m_BVH;
    PolarPerspectiveCamera m_Camera;

    // rendering input state
    ModelStateRenderParams m_RendererParams;
    ModelStateRenderer m_Rendererer;

    // ImGui compositing/hittesting state
    ImGuiImageDetails m_RenderImage;

    // other stuff
    bool m_AutoFocusCameraNextFrame = false;
    GuiRuler m_Ruler;
};


// public API

osc::UiModelViewer::UiModelViewer(UiModelViewerFlags flags) :
    m_Impl{new Impl{flags}}
{
}

osc::UiModelViewer::UiModelViewer(UiModelViewer&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::UiModelViewer& osc::UiModelViewer::operator=(UiModelViewer&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::UiModelViewer::~UiModelViewer() noexcept
{
    delete m_Impl;
}

bool osc::UiModelViewer::isLeftClicked() const
{
    return m_Impl->isLeftClicked();
}

bool osc::UiModelViewer::isRightClicked() const
{
    return m_Impl->isRightClicked();
}

bool osc::UiModelViewer::isMousedOver() const
{
    return m_Impl->isMousedOver();
}

void osc::UiModelViewer::requestAutoFocus()
{
    m_Impl->requestAutoFocus();
}

osc::UiModelViewerResponse osc::UiModelViewer::draw(VirtualConstModelStatePair const& rs)
{
    return m_Impl->draw(rs);
}
