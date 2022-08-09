#include "SceneRenderer.hpp"

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneDecorationFlags.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"
#include "src/Graphics/Texturing.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/Perf.hpp"

#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

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
            sceneFBO{[this]()
            {
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
        gl::VertexAttribPointer(mmtxAttr, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData) * offset + offsetof(SceneGPUInstanceData, modelMtx));
        gl::VertexAttribDivisor(mmtxAttr, 1);
        gl::EnableVertexAttribArray(mmtxAttr);

        gl::AttributeMat3 normMtxAttr{osc::SHADER_LOC_MATRIX_NORMAL};
        gl::VertexAttribPointer(normMtxAttr, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData) * offset + offsetof(SceneGPUInstanceData, normalMtx));
        gl::VertexAttribDivisor(normMtxAttr, 1);
        gl::EnableVertexAttribArray(normMtxAttr);

        gl::AttributeVec4 colorAttr{osc::SHADER_LOC_COLOR_DIFFUSE};
        gl::VertexAttribPointer(colorAttr, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData) * offset + offsetof(SceneGPUInstanceData, rgba));
        gl::VertexAttribDivisor(colorAttr, 1);
        gl::EnableVertexAttribArray(colorAttr);

        gl::AttributeFloat rimAttr{osc::SHADER_LOC_COLOR_RIM};
        gl::VertexAttribPointer(rimAttr, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData) * offset + offsetof(SceneGPUInstanceData, rimIntensity));
        gl::VertexAttribDivisor(rimAttr, 1);
        gl::EnableVertexAttribArray(rimAttr);
    }

    void PopulateGPUDrawlist(nonstd::span<osc::SceneDecoration const> in, std::vector<SceneGPUInstanceData>& out)
    {
        out.clear();
        out.reserve(in.size());

        // populate the list with the scene
        for (size_t i = 0; i < in.size(); ++i)
        {
            osc::SceneDecoration const& se = in[i];

            if (se.flags & (osc::SceneDecorationFlags_IsIsolated | osc::SceneDecorationFlags_IsChildOfIsolated))
            {
                continue;  // skip rendering this (it's not in the isolated component)
            }

            SceneGPUInstanceData& ins = out.emplace_back();
            ins.modelMtx = osc::ToMat4(se.transform);
            ins.normalMtx = osc::ToNormalMatrix(se.transform);
            ins.rgba = se.color;
            ins.decorationIdx = static_cast<int>(i);

            if (se.flags & (osc::SceneDecorationFlags_IsSelected | osc::SceneDecorationFlags_IsChildOfSelected))
            {
                ins.rimIntensity = 0.9f;
            }
            else if (se.flags & (osc::SceneDecorationFlags_IsHovered | osc::SceneDecorationFlags_IsChildOfHovered))
            {
                ins.rimIntensity = 0.4f;
            }
            else
            {
                ins.rimIntensity = 0.0f;
            }
        }
    }

    static bool HasGreaterAlphaOrLowerMeshID(
        osc::SceneDecoration const& a,
        osc::SceneDecoration const& b)
    {
        if (a.color.a != b.color.a)
        {
            // alpha descending, so non-opaque stuff is drawn last
            return a.color.a > b.color.a;
        }
        else
        {
            return a.mesh.get() < b.mesh.get();
        }
    }
}


class osc::SceneRenderer::Impl final {
public:
    glm::ivec2 getDimensions() const
    {
        return {m_RenderTexture.getWidth(), m_RenderTexture.getHeight()};
    }

    int getSamples() const
    {
        return m_RenderTexture.getAntialiasingLevel();
    }

    void draw(nonstd::span<SceneDecoration const> srcDecorations, SceneRendererParams const& params)
    {
        /*
        std::vector<SceneDecoration> decorations;
        {
            OSC_PERF("drawlist copying");
            decorations = std::vector<SceneDecoration>{srcDecorations.begin(), srcDecorations.end()};
        }

        {
            OSC_PERF("scene sorting");
            std::sort(decorations.begin(), decorations.end(), HasGreaterAlphaOrLowerMeshID);
        }

        // ensure underlying render target matches latest params
        m_RenderTarget.setDims(params.dimensions);
        m_RenderTarget.setSamples(params.samples);

        // populate GPU-friendly representation of the (higher-level) drawlist
        PopulateGPUDrawlist(decorations, m_GPUDrawlist);

        // upload data to the GPU
        gl::ArrayBuffer<SceneGPUInstanceData> instanceBuf{m_GPUDrawlist};

        // setup top-level OpenGL state
        gl::Viewport(0, 0, m_RenderTarget.dims.x, m_RenderTarget.dims.y);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        gl::Enable(GL_BLEND);
        gl::Enable(GL_DEPTH_TEST);
        gl::Disable(GL_SCISSOR_TEST);

        // draw scene
        {
            gl::BindFramebuffer(GL_FRAMEBUFFER, m_RenderTarget.sceneFBO);
            gl::ClearColor(params.backgroundColor);
            gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (params.wireframeMode)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }

            // draw the floor first, so that the transparent geometry in the scene also shows
            // the floor texture
            if (params.drawFloor)
            {
                auto& basicShader = osc::App::shader<osc::GouraudShader>();

                gl::UseProgram(basicShader.program);
                gl::Uniform(basicShader.uProjMat, params.projectionMatrix);
                gl::Uniform(basicShader.uViewMat, params.viewMatrix);
                glm::mat4 mtx = GenerateFloorModelMatrix(params.floorLocation, params.fixupScaleFactor);
                gl::Uniform(basicShader.uModelMat, mtx);
                gl::Uniform(basicShader.uNormalMat, osc::ToNormalMatrix(mtx));
                gl::Uniform(basicShader.uLightDir, params.lightDirection);
                gl::Uniform(basicShader.uLightColor, params.lightColor);
                gl::Uniform(basicShader.uViewPos, params.viewPos);
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
            gl::Uniform(instancedShader.uProjMat, params.projectionMatrix);
            gl::Uniform(instancedShader.uViewMat, params.viewMatrix);
            gl::Uniform(instancedShader.uLightDir, params.lightDirection);
            gl::Uniform(instancedShader.uLightColor, params.lightColor);
            gl::Uniform(instancedShader.uViewPos, params.viewPos);

            nonstd::span<SceneGPUInstanceData const> instances = m_GPUDrawlist;

            size_t pos = 0;
            size_t ninstances = instances.size();

            while (pos < ninstances)
            {
                osc::SceneDecoration const& se = decorations[instances[pos].decorationIdx];

                // batch
                size_t end = pos + 1;
                while (end < ninstances &&
                    decorations[instances[end].decorationIdx].mesh == se.mesh)
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

            if (params.wireframeMode)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
        }

        // draw mesh normals, if requested
        if (params.drawMeshNormals)
        {
            auto& normalShader = osc::App::shader<osc::NormalsShader>();
            gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
            gl::UseProgram(normalShader.program);
            gl::Uniform(normalShader.uProjMat, params.projectionMatrix);
            gl::Uniform(normalShader.uViewMat, params.viewMatrix);

            for (SceneGPUInstanceData const& inst : m_GPUDrawlist)
            {
                osc::SceneDecoration const& se = decorations[inst.decorationIdx];

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
        if (params.drawRims)
        {
            gl::BindFramebuffer(GL_FRAMEBUFFER, m_RenderTarget.rimsFBO);
            gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto& iscs = osc::App::shader<osc::InstancedSolidColorShader>();
            gl::UseProgram(iscs.program);
            gl::Uniform(iscs.uVP, params.projectionMatrix * params.viewMatrix);

            size_t pos = 0;
            size_t ninstances = m_GPUDrawlist.size();

            // drawcalls & figure out rim AABB
            osc::AABB rimAABB{};
            rimAABB.min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
            rimAABB.max = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
            bool hasRims = false;

            while (pos < ninstances)
            {
                SceneGPUInstanceData const& inst = m_GPUDrawlist[pos];
                osc::SceneDecoration const& se = decorations[inst.decorationIdx];

                // batch
                size_t end = pos + 1;
                while (end < ninstances &&
                    decorations[m_GPUDrawlist[end].decorationIdx].mesh == se.mesh &&
                    m_GPUDrawlist[end].rimIntensity == inst.rimIntensity)
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
                    rimAABB = Union(rimAABB, GetWorldspaceAABB(decorations[m_GPUDrawlist[i].decorationIdx]));
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
                osc::AABB screenspaceRimBounds = TransformAABB(rimAABB, params.projectionMatrix * params.viewMatrix);
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
        */
    }

    experimental::RenderTexture& updRenderTexture()
    {
        return m_RenderTexture;
    }

private:
    gl::Texture2D m_ChequerTexture = GenChequeredFloorTexture();
    experimental::RenderTexture m_RenderTexture{experimental::RenderTextureDescriptor{1, 1}};
};

osc::SceneRenderer::SceneRenderer() :
    m_Impl{new Impl{}}
{
}

osc::SceneRenderer::SceneRenderer(SceneRenderer && tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SceneRenderer& osc::SceneRenderer::operator=(SceneRenderer&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::SceneRenderer::~SceneRenderer() noexcept
{
    delete m_Impl;
}

glm::ivec2 osc::SceneRenderer::getDimensions() const
{
    return m_Impl->getDimensions();
}

int osc::SceneRenderer::getSamples() const
{
    return m_Impl->getSamples();
}

void osc::SceneRenderer::draw(nonstd::span<SceneDecoration const> decs, SceneRendererParams const& params)
{
    m_Impl->draw(std::move(decs), params);
}

osc::experimental::RenderTexture& osc::SceneRenderer::updRenderTexture()
{
    return m_Impl->updRenderTexture();
}
