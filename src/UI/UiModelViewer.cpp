#include "src/UI/UiModelViewer.hpp"

#include "src/3D/Shaders/GouraudMrtShader.hpp"
#include "src/3D/Shaders/NormalsShader.hpp"
#include "src/3D/Shaders/EdgeDetectionShader.hpp"
#include "src/3D/Shaders/SolidColorShader.hpp"
#include "src/3D/Constants.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/GlGlm.hpp"
#include "src/3D/Texturing.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/App.hpp"
#include "src/Log.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <OpenSim/Common/Component.h>

#include <unordered_map>

using namespace osc;

namespace {

    // helper method for making a render buffer (used in Render_target)
    gl::RenderBuffer makeRenderBuffer(int samples, GLenum format, int w, int h) {
        gl::RenderBuffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, w, h);
        return rv;
    }

    // draw targets written to by the renderer
    struct RenderTarget final {
        glm::ivec2 dims;
        int samples;

        // internally used for initial render pass
        gl::RenderBuffer sceneMsxaaRb;
        gl::RenderBuffer rimsMsxaaRb;
        gl::RenderBuffer depth24Stencil8Rb;
        gl::FrameBuffer renderMsxaaFbo;

        // internally used to blit the solid rims (before edge-detection) into
        // a cheaper-to-sample not-multisampled texture
        gl::Texture2D rimsTex;
        gl::FrameBuffer rimsTexFbo;

        // these are the actual outputs
        gl::Texture2D outputTex;
        gl::Texture2D outputDepth24Stencil8Tex;
        gl::FrameBuffer outputFbo;

        RenderTarget(glm::ivec2 dims_, int samples_) :
            dims{dims_},
            samples{samples_},

            sceneMsxaaRb{makeRenderBuffer(samples, GL_RGBA, dims.x, dims.y)},
            rimsMsxaaRb{makeRenderBuffer(samples, GL_RED, dims.x, dims.y)},
            depth24Stencil8Rb{makeRenderBuffer(samples, GL_DEPTH24_STENCIL8, dims.x, dims.y)},
            renderMsxaaFbo{[this]() {
                gl::FrameBuffer rv;
                gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sceneMsxaaRb);
                gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, rimsMsxaaRb);
                gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depth24Stencil8Rb);
                gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
                return rv;
            }()},

            rimsTex{[this]() {
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
            rimsTexFbo{[this]() {
                gl::FrameBuffer rv;
                gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rimsTex, 0);
                gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
                return rv;
            }()},

            outputTex{[this]() {
                gl::Texture2D rv;
                gl::BindTexture(rv);
                gl::TexImage2D(rv.type, 0, GL_RGBA, dims.x, dims.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
                gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
                return rv;
            }()},
            outputDepth24Stencil8Tex{[this]() {
                gl::Texture2D rv;
                gl::BindTexture(rv);
                // https://stackoverflow.com/questions/27535727/opengl-create-a-depth-stencil-texture-for-reading
                gl::TexImage2D(rv.type, 0, GL_DEPTH24_STENCIL8, dims.x, dims.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
                return rv;
            }()},
            outputFbo{[this]() {
                gl::FrameBuffer rv;
                gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, outputTex, 0);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, outputDepth24Stencil8Tex, 0);
                gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
                return rv;
            }()} {
        }

        void setDims(glm::ivec2 newDims) {
            if (newDims != dims) {
                *this = RenderTarget{newDims, samples};
            }
        }

        void setSamples(int newSamples) {
            if (samples != newSamples) {
                *this = RenderTarget{dims, newSamples};
            }
        }
    };

    struct SceneGPUElementData final {
        glm::vec3 pos;
        glm::vec3 norm;
    };

    gl::ArrayBuffer<SceneGPUElementData> uploadMeshToGPU(Mesh const& m) {
        OSC_ASSERT_ALWAYS(m.verts.size() == m.normals.size());

        std::vector<SceneGPUElementData> buf;
        buf.reserve(m.verts.size());

        for (size_t i = 0; i < m.verts.size(); ++i) {
            buf.push_back({m.verts[i], m.normals[i]});
        }

        return {buf};
    }

    gl::VertexArray makeVAO(gl::ArrayBuffer<SceneGPUElementData>& vbo,
                            gl::ElementArrayBuffer<GLushort>& ebo) {

        gl::VertexArray rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(vbo);
        gl::BindBuffer(ebo);
        gl::VertexAttribPointer(gl::AttributeVec3{0}, false, sizeof(SceneGPUElementData), offsetof(SceneGPUElementData, pos));
        gl::EnableVertexAttribArray(gl::AttributeVec3{0});
        gl::VertexAttribPointer(gl::AttributeVec3{2}, false, sizeof(SceneGPUElementData), offsetof(SceneGPUElementData, norm));
        gl::EnableVertexAttribArray(gl::AttributeVec3{2});
        return rv;
    }

    struct SceneGPUMesh final {
        gl::ArrayBuffer<SceneGPUElementData> data;
        gl::ElementArrayBuffer<GLushort> indices;
        gl::VertexArray vao;

        SceneGPUMesh(Mesh const& m) :
            data{uploadMeshToGPU(m)},
            indices{m.indices},
            vao{makeVAO(data, indices)} {
        }
    };

    struct SceneGPUInstanceData final {
        glm::mat4x3 modelMtx;
        glm::mat3 normalMtx;
        glm::vec4 rgba;
        float rimIntensity;
    };

    // GPU format of meshdata with texcoords
    struct GPUTexturedMeshdata final {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 uv;
    };

    gl::ArrayBuffer<GPUTexturedMeshdata> generateQuadVBO() {
        Mesh m = GenTexturedQuad();

        std::vector<GPUTexturedMeshdata> swap;
        for (size_t i = 0; i < m.indices.size(); ++i) {
            unsigned short idx = m.indices[i];
            GPUTexturedMeshdata& tv = swap.emplace_back();
            tv.pos = m.verts[idx];
            tv.norm = m.normals[idx];
            tv.uv = m.texcoords[idx];
        }

        return gl::ArrayBuffer<GPUTexturedMeshdata>{swap};
    }

    static gl::ArrayBuffer<GPUTexturedMeshdata> generateFloorVBO() {
        Mesh m = GenTexturedQuad();
        for (auto& coord : m.texcoords) {
            coord *= 200.0f;
        }

        std::vector<GPUTexturedMeshdata> swap;
        for (size_t i = 0; i < m.indices.size(); ++i) {
            auto idx = m.indices[i];
            auto& el = swap.emplace_back();
            el.pos = m.verts[idx];
            el.norm = m.verts[idx];
            el.uv = m.texcoords[idx];
        }

        return {swap};
    }

    gl::VertexArray makeEdgeDetectionVAO(gl::ArrayBuffer<GPUTexturedMeshdata>& vbo) {
        gl::VertexArray rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(EdgeDetectionShader::aPos, false, sizeof(GPUTexturedMeshdata), offsetof(GPUTexturedMeshdata, pos));
        gl::EnableVertexAttribArray(EdgeDetectionShader::aPos);
        gl::VertexAttribPointer(EdgeDetectionShader::aTexCoord, false, sizeof(GPUTexturedMeshdata), offsetof(GPUTexturedMeshdata, uv));
        gl::EnableVertexAttribArray(EdgeDetectionShader::aTexCoord);
        return rv;
    }

    gl::VertexArray makeFloorVAO(gl::ArrayBuffer<GPUTexturedMeshdata>& vbo) {

        gl::VertexArray rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(gl::AttributeVec3{0}, false, sizeof(GPUTexturedMeshdata), offsetof(GPUTexturedMeshdata, pos));
        gl::EnableVertexAttribArray(gl::AttributeVec3{0});
        gl::VertexAttribPointer(gl::AttributeVec2{1}, false, sizeof(GPUTexturedMeshdata), offsetof(GPUTexturedMeshdata, uv));
        gl::EnableVertexAttribArray(gl::AttributeVec3{1});
        gl::VertexAttribPointer(gl::AttributeVec3{2}, false, sizeof(GPUTexturedMeshdata), offsetof(GPUTexturedMeshdata, norm));
        gl::EnableVertexAttribArray(gl::AttributeVec3{2});
        return rv;
    }
}

static gl::VertexArray makeSCSVAO(SolidColorShader& shader, gl::ArrayBuffer<glm::vec3>& vbo) {
    gl::VertexArray rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(shader.aPos);
    return rv;
}

struct osc::UiModelViewer::Impl final {
    std::unordered_map<SceneMesh::IdType, std::unique_ptr<SceneGPUMesh>> gpuCache;
    GouraudMrtShader shader;
    NormalsShader normalsShader;
    EdgeDetectionShader edgeDetectionShader;
    SolidColorShader solidColorShader;

    UiModelViewerFlags flags;
    PolarPerspectiveCamera camera;
    glm::vec3 lightDir = {-0.34f, -0.25f, 0.05f};
    glm::vec3 lightCol = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    glm::vec4 backgroundCol = {0.89f, 0.89f, 0.89f, 1.0f};
    glm::vec4 rimCol = {1.0f, 0.4f, 0.0f, 0.85f};

    RenderTarget renderTarg{{1, 1}, 1};

    gl::ArrayBuffer<GPUTexturedMeshdata> quadVBO = generateQuadVBO();
    gl::VertexArray quadVAO = makeEdgeDetectionVAO(quadVBO);

    gl::ArrayBuffer<GPUTexturedMeshdata> floorVBO = generateFloorVBO();
    gl::VertexArray floorVAO = makeFloorVAO(floorVBO);
    gl::Texture2D chequerTex = genChequeredFloorTexture();

    // grid data
    gl::ArrayBuffer<glm::vec3> gridVBO{GenNbyNGrid(100).verts};
    gl::VertexArray gridVAO = makeSCSVAO(solidColorShader, gridVBO);

    // line data
    gl::ArrayBuffer<glm::vec3> lineVBO{GenYLine().verts};
    gl::VertexArray lineVAO = makeSCSVAO(solidColorShader, lineVBO);

    // aabb data
    gl::ArrayBuffer<glm::vec3> cubewireVBO{GenCubeLines().verts};
    gl::VertexArray cubewireVAO = makeSCSVAO(solidColorShader, cubewireVBO);

    std::vector<BVHCollision> sceneHittestResults;
    std::vector<BVHCollision> triangleHittestResults;

    glm::vec2 renderDims = {0.0f, 0.0f};
    bool renderHovered = false;
    bool renderLeftClicked = false;
    bool renderRightClicked = false;
    bool wireframeMode = false;
    bool drawMeshNormals = false;
    bool drawRims = true;

    std::vector<float> rimHighlights;
    std::vector<SceneGPUInstanceData> buf;

    Impl(UiModelViewerFlags flags_) : flags{flags_} {
    }

    SceneGPUMesh& getGPUMeshCached(SceneMesh const& se) {
        auto [it, inserted] = gpuCache.try_emplace(se.getID(), nullptr);
        if (inserted) {
            it->second = std::make_unique<SceneGPUMesh>(se.getMesh());
        }
        return *it->second;
    }

    gl::ArrayBuffer<SceneGPUInstanceData> uploadInstances(RenderableScene const& rs) {
        auto decs = rs.getSceneDecorations();

        rimHighlights.resize(decs.size(), 0.0f);  // asssume they're pre-populated but ensure this minimum size
        buf.clear();
        buf.reserve(decs.size());
        for (size_t i = 0; i < decs.size(); ++i) {
            LabelledSceneElement const& se = decs[i];
            buf.push_back({se.modelMtx, se.normalMtx, se.color, rimHighlights[i]});
        }
        return {buf};
    }
};

osc::UiModelViewer::UiModelViewer(UiModelViewerFlags flags) :
    m_Impl{new Impl{flags}} {
}

osc::UiModelViewer::~UiModelViewer() noexcept = default;

bool osc::UiModelViewer::isMousedOver() const noexcept {
    return m_Impl->renderHovered;
}

static glm::mat4x3 generateFloorModelMatrix(RenderableScene const& rs) {
    float fixupScaleFactor = rs.getFixupScaleFactor();

    // rotate from XY (+Z dir) to ZY (+Y dir)
    glm::mat4 rv = glm::rotate(glm::mat4{1.0f}, -fpi2, {1.0f, 0.0f, 0.0f});

    // make floor extend far in all directions
    rv = glm::scale(glm::mat4{1.0f}, {fixupScaleFactor * 100.0f, 1.0f, fixupScaleFactor * 100.0f}) * rv;

    // lower slightly, so that it doesn't conflict with OpenSim model planes
    // that happen to lie at Z==0
    rv = glm::translate(glm::mat4{1.0f}, {0.0f, -0.0001f, 0.0f}) * rv;

    return glm::mat4x3{rv};
}

static void drawFloor(osc::UiModelViewer::Impl& impl, RenderableScene const& rs) {


    glm::mat4x3 modelMtx = generateFloorModelMatrix(rs);
    glm::vec4 color = {0.0f, 0.0f, 0.0f, 0.0f};
    // TODO
/*


    decs.normalMtxs.push_back(NormalMatrix(modelXform));
    decs.cols.push_back(Rgba32{0x00, 0x00, 0x00, 0x00});
    decs.gpuMeshes.push_back(impl.floorMeshdata);
    decs.cpuMeshes.push_back(impl.floorMesh);
    decs.aabbs.push_back(AABBApplyXform(impl.floorMesh->aabb, modelXform));
    decs.components.push_back(nullptr);

    impl.textures.clear();
    impl.textures.resize(decs.modelMtxs.size(), nullptr);
    impl.textures.back() = impl.chequerTex;
    */
}

static AABB computeWorldspaceRimAABB(osc::UiModelViewer::Impl& impl, RenderableScene const& rs) {
    auto decs = rs.getSceneDecorations();

    impl.rimHighlights.resize(decs.size(), 0.0f);  // safety

    if (decs.empty()) {
        return {};
    }

    AABB rv;
    rv.min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    rv.max = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

    int n = 0;
    for (size_t i = 0; i < decs.size(); ++i) {
        if (impl.rimHighlights[i] > 0.0f) {
            rv = AABBUnion(decs[i].worldspaceAABB, rv);
            ++n;
        }
    }

    if (n == 0) {
        return {};
    }

    return rv;
}

static void drawSceneTexture(osc::UiModelViewer::Impl& impl, RenderableScene const& rs) {

    // resize output texture to match this output
    // render scene with renderer
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    if (contentRegion.x >= 1.0f && contentRegion.y >= 1.0f) {

        // ensure renderer dims match the panel's dims
        glm::ivec2 dims{static_cast<int>(contentRegion.x), static_cast<int>(contentRegion.y)};
        impl.renderTarg.setDims(dims);
        impl.renderTarg.setSamples(App::cur().getSamples());
    }

    // setup FBOs and textures for new render drawcalls
    gl::Viewport(0, 0, impl.renderTarg.dims.x, impl.renderTarg.dims.y);
    gl::BindFramebuffer(GL_FRAMEBUFFER, impl.renderTarg.renderMsxaaFbo);
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::ClearColor(impl.backgroundCol);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::DrawBuffer(GL_COLOR_ATTACHMENT1);
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT);
    gl::DrawBuffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnablei(GL_BLEND, 0);
    glDisablei(GL_BLEND, 1);

    if (impl.wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glm::mat4 projMtx = impl.camera.getProjMtx(static_cast<float>(impl.renderTarg.dims.x)/static_cast<float>(impl.renderTarg.dims.y));
    glm::mat4 viewMtx = impl.camera.getViewMtx();
    glm::vec3 viewerPos = impl.camera.getPos();

    // setup uniforms
    GouraudMrtShader& shader = impl.shader;
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjMat, projMtx);
    gl::Uniform(shader.uViewMat, viewMtx);
    gl::Uniform(shader.uLightDir, impl.lightDir);
    gl::Uniform(shader.uIsTextured, false);
    gl::Uniform(shader.uLightColor, impl.lightCol);
    gl::Uniform(shader.uViewPos, viewerPos);

    // upload all instances to the GPU
    gl::ArrayBuffer instanceBuf = impl.uploadInstances(rs);
    auto decs = rs.getSceneDecorations();

    // draw scene
    {
        size_t pos = 0;
        size_t ninstances = decs.size();

        while (pos < ninstances) {
            auto const& se = decs[pos];

            // batch
            size_t end = pos + 1;
            while (end < ninstances && decs[end].mesh.get() == se.mesh.get()) {
                ++end;
            }

            // lookup/populate GPU data for mesh
            SceneGPUMesh const& gpuMesh = impl.getGPUMeshCached(*se.mesh);

            gl::BindVertexArray(gpuMesh.vao);
            gl::VertexAttribPointer(GouraudMrtShader::aModelMat, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*pos + offsetof(SceneGPUInstanceData, modelMtx));
            gl::VertexAttribDivisor(GouraudMrtShader::aModelMat, 1);
            gl::EnableVertexAttribArray(GouraudMrtShader::aModelMat);
            gl::VertexAttribPointer(GouraudMrtShader::aNormalMat, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*pos + offsetof(SceneGPUInstanceData, normalMtx));
            gl::VertexAttribDivisor(GouraudMrtShader::aNormalMat, 1);
            gl::EnableVertexAttribArray(GouraudMrtShader::aNormalMat);
            gl::VertexAttribPointer(GouraudMrtShader::aDiffuseColor, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*pos + offsetof(SceneGPUInstanceData, rgba));
            gl::VertexAttribDivisor(GouraudMrtShader::aDiffuseColor, 1);
            gl::EnableVertexAttribArray(GouraudMrtShader::aDiffuseColor);
            gl::VertexAttribPointer(GouraudMrtShader::aRimIntensity, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*pos + offsetof(SceneGPUInstanceData, rimIntensity));
            gl::VertexAttribDivisor(GouraudMrtShader::aRimIntensity, 1);
            gl::EnableVertexAttribArray(GouraudMrtShader::aRimIntensity);
            glDrawElementsInstanced(GL_TRIANGLES, gpuMesh.indices.sizei(), gl::indexType(gpuMesh.indices), nullptr, static_cast<GLsizei>(end-pos));
            gl::BindVertexArray();

            pos = end;
        }
    }

    drawFloor(impl, rs);

    if (impl.wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // draw mesh normals, if requested
    if (impl.drawMeshNormals) {
        NormalsShader& shader = impl.normalsShader;
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, projMtx);
        gl::Uniform(shader.uViewMat, viewMtx);
        for (auto const& se : decs) {
            gl::Uniform(shader.uModelMat, se.modelMtx);
            gl::Uniform(shader.uNormalMat, se.normalMtx);
            SceneGPUMesh const& gpuMesh = impl.getGPUMeshCached(*se.mesh);
            gl::BindVertexArray(gpuMesh.vao);
            gl::DrawElements(GL_TRIANGLES, gpuMesh.indices.sizei(), gl::indexType(gpuMesh.indices), nullptr);
        }
        gl::BindVertexArray();
    }

    // blit render to non-MSXAAed output texture
    gl::BindFramebuffer(GL_READ_FRAMEBUFFER, impl.renderTarg.renderMsxaaFbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, impl.renderTarg.outputFbo);
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::BlitFramebuffer(0, 0, impl.renderTarg.dims.x, impl.renderTarg.dims.y, 0, 0, impl.renderTarg.dims.x, impl.renderTarg.dims.y, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    // handle blitting rims onto output
    if (impl.drawRims) {

        float rimThickness = 2.0f / std::max(impl.renderTarg.dims.x, impl.renderTarg.dims.y);

        // calculate a screenspace bounding box that surrounds the rims so that the
        // edge detection shader only had to run on a smaller subset of the screen
        AABB worldspaceRimElsBounds = computeWorldspaceRimAABB(impl, rs);
        AABB screenspaceRimBounds = AABBApplyXform(worldspaceRimElsBounds, projMtx * viewMtx);
        auto verts = AABBVerts(screenspaceRimBounds);
        glm::vec2 bounds[2] = {{verts[0].x, verts[0].y}, {verts[0].x, verts[0].y}};
        for (size_t i = 1; i < verts.size(); ++i) {
            glm::vec2 p{verts[i].x, verts[i].y};
            bounds[0] = VecMin(p, bounds[0]);
            bounds[1] = VecMax(p, bounds[1]);
        }
        bounds[0] -= rimThickness;
        bounds[1] += rimThickness;
        glm::ivec2 renderDims = impl.renderTarg.dims;
        glm::ivec2 min{
            static_cast<int>((bounds[0].x + 1.0f)/2.0f * renderDims.x),
            static_cast<int>((bounds[0].y + 1.0f)/2.0f * renderDims.y)};
        glm::ivec2 max{
            static_cast<int>((bounds[1].x + 1.0f)/2.0f * renderDims.x),
            static_cast<int>((bounds[1].y + 1.0f)/2.0f * renderDims.y)};

        int x = std::max(0, min.x);
        int y = std::max(0, min.y);
        int w = max.x - min.x;
        int h = max.y - min.y;

        // blit rims from MSXAAed (expensive to sample) texture to a standard
        // not-MSXAAed texture
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, impl.renderTarg.renderMsxaaFbo);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, impl.renderTarg.rimsTexFbo);
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT);
        gl::BlitFramebuffer(min.x, min.y, max.x, max.y, min.x, min.y, max.x, max.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // set shader to write directly to output
        gl::BindFramebuffer(GL_FRAMEBUFFER, impl.renderTarg.outputFbo);
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);

        // setup edge-detection shader
        EdgeDetectionShader& shader = impl.edgeDetectionShader;
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uModelMat, gl::identity);
        gl::Uniform(shader.uViewMat, gl::identity);
        gl::Uniform(shader.uProjMat, gl::identity);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(impl.renderTarg.rimsTex);
        gl::Uniform(shader.uSampler0, gl::textureIndex<GL_TEXTURE0>());
        gl::Uniform(shader.uRimRgba, {1.0f, 0.4f, 0.0f, 0.85f});
        gl::Uniform(shader.uRimThickness, rimThickness);

        // draw edges, directly writing into output texture
        gl::Enable(GL_SCISSOR_TEST);
        glScissor(x, y, w, h);
        gl::Enable(GL_BLEND);
        gl::Disable(GL_DEPTH_TEST);
        gl::BindVertexArray(impl.quadVAO);
        gl::DrawArrays(GL_TRIANGLES, 0, impl.quadVBO.sizei());
        gl::BindVertexArray();
        gl::Enable(GL_DEPTH_TEST);
        gl::Disable(GL_SCISSOR_TEST);
    }

    // rebind to main window FBO
    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
}

static void blitSceneTexture(osc::UiModelViewer::Impl& impl) {
    GLint texGLHandle = impl.renderTarg.outputTex.get();
    void* texImGuiHandle = reinterpret_cast<void*>(static_cast<uintptr_t>(texGLHandle));

    ImVec2 imgDims = ImGui::GetContentRegionAvail();
    ImVec2 texcoordUV0 = {0.0f, 1.0f};
    ImVec2 texcoordUV1 = {1.0f, 0.0f};

    ImGui::Image(texImGuiHandle, imgDims, texcoordUV0, texcoordUV1);

    impl.renderDims = ImGui::GetItemRectSize();
    impl.renderHovered = ImGui::IsItemHovered();
    impl.renderLeftClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    impl.renderRightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
}

static OpenSim::Component const* hittestSceneDecorations(osc::UiModelViewer::Impl& impl,
                                                         RenderableScene const& rs) {
    if (!impl.renderHovered) {
        return nullptr;
    }

    // figure out mouse pos in panel's NDC system
    glm::vec2 windowScreenPos = ImGui::GetWindowPos();  // where current ImGui window is in the screen
    glm::vec2 mouseScreenPos = ImGui::GetMousePos();  // where mouse is in the screen
    glm::vec2 mouseWindowPos = mouseScreenPos - windowScreenPos;  // where mouse is in current window
    glm::vec2 cursorWindowPos = ImGui::GetCursorPos();  // where cursor is in current window
    glm::vec2 mouseItemPos = mouseWindowPos - cursorWindowPos;  // where mouse is in current item
    glm::vec2 itemDims = ImGui::GetContentRegionAvail();  // how big current window will be

    // un-project the mouse position as a ray in worldspace
    Line cameraRay = impl.camera.unprojectScreenposToWorldRay(mouseItemPos, itemDims);

    // use scene BVH to intersect that ray with the scene
    impl.sceneHittestResults.clear();
    BVH_GetRayAABBCollisions(rs.getSceneBVH(), cameraRay, &impl.sceneHittestResults);

    // go through triangle BVHes to figure out which, if any, triangle is closest intersecting
    int closestIdx = -1;
    float closestDistance = std::numeric_limits<float>::max();

    // iterate through each scene-level hit and perform a triangle-level hittest
    auto decs = rs.getSceneDecorations();
    for (BVHCollision const& c : impl.sceneHittestResults) {
        int instanceIdx = c.primId;
        glm::mat4 instanceMmtx = decs[instanceIdx].modelMtx;
        SceneMesh const& instanceMesh = *decs[instanceIdx].mesh;

        Line cameraRayModelspace = LineApplyXform(cameraRay, glm::inverse(instanceMmtx));

        // perform ray-triangle BVH hittest
        impl.triangleHittestResults.clear();
        BVH_GetRayTriangleCollisions(
                    instanceMesh.getTriangleBVH(),
                    instanceMesh.getVerts().data(),
                    instanceMesh.getVerts().size(),
                    cameraRayModelspace,
                    &impl.triangleHittestResults);

        // check each triangle collision and take the closest
        for (BVHCollision const& tc : impl.triangleHittestResults) {
            if (tc.distance < closestDistance) {
                closestIdx = instanceIdx;
                closestDistance = tc.distance;
            }
        }
    }

    return closestIdx >= 0 ? decs[closestIdx].component : nullptr;
}

static void drawXZGrid(osc::UiModelViewer::Impl& impl) {
    SolidColorShader& shader = impl.solidColorShader;

    gl::UseProgram(shader.program);
    gl::Uniform(shader.uModel, glm::scale(glm::rotate(glm::mat4{1.0f}, fpi2, {1.0f, 0.0f, 0.0f}), {5.0f, 5.0f, 1.0f}));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
    gl::BindVertexArray(impl.gridVAO);
    gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    gl::BindVertexArray();
}

static void drawXYGrid(osc::UiModelViewer::Impl& impl) {
    SolidColorShader& shader = impl.solidColorShader;

    gl::UseProgram(shader.program);
    gl::Uniform(shader.uModel, glm::scale(glm::mat4{1.0f}, {5.0f, 5.0f, 1.0f}));
    gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::BindVertexArray(impl.gridVAO);
    gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    gl::BindVertexArray();
}

static void drawYZGrid(osc::UiModelViewer::Impl& impl) {
    SolidColorShader& shader = impl.solidColorShader;

    gl::UseProgram(shader.program);
    gl::Uniform(shader.uModel, glm::scale(glm::rotate(glm::mat4{1.0f}, fpi2, {0.0f, 1.0f, 0.0f}), {5.0f, 5.0f, 1.0f}));
    gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::BindVertexArray(impl.gridVAO);
    gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    gl::BindVertexArray();
}

static void drawAlignmentAxes(osc::UiModelViewer::Impl& impl) {
    glm::mat4 model2view = impl.camera.getViewMtx();

    // we only care about rotation of the axes, not translation
    model2view[3] = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};

    // rescale + translate the y-line vertices
    glm::mat4 makeLineOneSided = glm::translate(glm::identity<glm::mat4>(), glm::vec3{0.0f, 1.0f, 0.0f});
    glm::mat4 scaler = glm::scale(glm::identity<glm::mat4>(), glm::vec3{0.025f});
    glm::mat4 translator = glm::translate(glm::identity<glm::mat4>(), glm::vec3{-0.95f, -0.95f, 0.0f});
    glm::mat4 baseModelMtx = translator * scaler * model2view;

    SolidColorShader& shader = impl.solidColorShader;

    // common shader stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, gl::identity);
    gl::Uniform(shader.uView, gl::identity);
    gl::BindVertexArray(impl.lineVAO);
    gl::Disable(GL_DEPTH_TEST);

    // y axis
    {
        gl::Uniform(shader.uColor, {0.0f, 1.0f, 0.0f, 1.0f});
        gl::Uniform(shader.uModel, baseModelMtx * makeLineOneSided);
        gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    }

    // x axis
    {
        glm::mat4 rotateYtoX =
            glm::rotate(glm::identity<glm::mat4>(), fpi2, glm::vec3{0.0f, 0.0f, -1.0f});

        gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
        gl::Uniform(shader.uModel, baseModelMtx * rotateYtoX * makeLineOneSided);
        gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    }

    // z axis
    {
        glm::mat4 rotateYtoZ =
            glm::rotate(glm::identity<glm::mat4>(), fpi2, glm::vec3{1.0f, 0.0f, 0.0f});

        gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
        gl::Uniform(shader.uModel, baseModelMtx * rotateYtoZ * makeLineOneSided);
        gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    }

    gl::BindVertexArray();
    gl::Enable(GL_DEPTH_TEST);
}

static void drawFloorAxesLines(osc::UiModelViewer::Impl& impl) {
    SolidColorShader& shader = impl.solidColorShader;

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::BindVertexArray(impl.lineVAO);

    // X
    gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, fpi2, {0.0f, 0.0f, 1.0f}));
    gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
    gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());

    // Z
    gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, fpi2, {1.0f, 0.0f, 0.0f}));
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
    gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());

    gl::BindVertexArray();
}

static void drawAABBs(osc::UiModelViewer::Impl& impl, RenderableScene const& rs) {
    SolidColorShader& shader = impl.solidColorShader;

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

    gl::BindVertexArray(impl.cubewireVAO);
    for (auto const& se : rs.getSceneDecorations()) {
        glm::vec3 halfWidths = AABBDims(se.worldspaceAABB) / 2.0f;
        glm::vec3 center = AABBCenter(se.worldspaceAABB);

        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
        glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
        glm::mat4 mmtx = mover * scaler;

        gl::Uniform(shader.uModel, mmtx);
        gl::DrawArrays(GL_LINES, 0, impl.cubewireVBO.sizei());
    }
    gl::BindVertexArray();
}

// assumes `pos` is in-bounds
static void drawBVHRecursive(osc::UiModelViewer::Impl& impl, BVH const& bvh, int pos) {
    BVHNode const& n = bvh.nodes[pos];

    glm::vec3 halfWidths = AABBDims(n.bounds) / 2.0f;
    glm::vec3 center = AABBCenter(n.bounds);

    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
    glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
    glm::mat4 mmtx = mover * scaler;
    gl::Uniform(impl.solidColorShader.uModel, mmtx);
    gl::DrawArrays(GL_LINES, 0, impl.cubewireVBO.sizei());

    if (n.nlhs >= 0) {  // if it's an internal node
        drawBVHRecursive(impl, bvh, pos+1);
        drawBVHRecursive(impl, bvh, pos+n.nlhs+1);
    }
}

static void drawBVH(osc::UiModelViewer::Impl& impl, RenderableScene const& rs) {
    BVH const& bvh = rs.getSceneBVH();

    if (bvh.nodes.empty()) {
        return;
    }

    SolidColorShader& shader = impl.solidColorShader;

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

    gl::BindVertexArray(impl.cubewireVAO);
    drawBVHRecursive(impl, bvh, 0);
    gl::BindVertexArray();
}

static void drawOverlays(osc::UiModelViewer::Impl& impl, RenderableScene const& rs) {
    gl::BindFramebuffer(GL_FRAMEBUFFER, impl.renderTarg.outputFbo);
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);

    if (impl.flags & RenderableSceneViewerFlags_DrawXZGrid) {
        drawXZGrid(impl);
    }

    if (impl.flags & RenderableSceneViewerFlags_DrawXYGrid) {
        drawXYGrid(impl);
    }

    if (impl.flags & RenderableSceneViewerFlags_DrawYZGrid) {
        drawYZGrid(impl);
    }

    if (impl.flags & RenderableSceneViewerFlags_DrawAlignmentAxes) {
        drawAlignmentAxes(impl);
    }

    if (impl.flags & RenderableSceneViewerFlags_DrawAxisLines) {
        drawFloorAxesLines(impl);
    }

    if (impl.flags & RenderableSceneViewerFlags_DrawAABBs) {
        drawAABBs(impl, rs);
    }

    if (impl.flags & RenderableSceneViewerFlags_DrawBVH) {
        drawBVH(impl, rs);
    }

    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
}

static void drawOptionsMenu(osc::UiModelViewer::Impl& impl) {
    ImGui::Checkbox("wireframe mode", &impl.wireframeMode);
    ImGui::Checkbox("show normals", &impl.drawMeshNormals);
    ImGui::Checkbox("draw rims", &impl.drawRims);
    ImGui::CheckboxFlags("show XZ grid", &impl.flags, RenderableSceneViewerFlags_DrawXZGrid);
    ImGui::CheckboxFlags("show XY grid", &impl.flags, RenderableSceneViewerFlags_DrawXYGrid);
    ImGui::CheckboxFlags("show YZ grid", &impl.flags, RenderableSceneViewerFlags_DrawYZGrid);
    ImGui::CheckboxFlags("show alignment axes", &impl.flags, RenderableSceneViewerFlags_DrawAlignmentAxes);
    ImGui::CheckboxFlags("show grid lines", &impl.flags, RenderableSceneViewerFlags_DrawAxisLines);
    ImGui::CheckboxFlags("show AABBs", &impl.flags, RenderableSceneViewerFlags_DrawAABBs);
    ImGui::CheckboxFlags("show BVH", &impl.flags, RenderableSceneViewerFlags_DrawBVH);
}

static void drawSceneMenu(osc::UiModelViewer::Impl& impl) {
    if (ImGui::Button("Top")) {
        impl.camera.theta = 0.0f;
        impl.camera.phi = fpi2;
    }

    if (ImGui::Button("Left")) {
        // assumes models tend to point upwards in Y and forwards in +X
        // (so sidewards is theta == 0 or PI)
        impl.camera.theta = fpi;
        impl.camera.phi = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Right")) {
        // assumes models tend to point upwards in Y and forwards in +X
        // (so sidewards is theta == 0 or PI)
        impl.camera.theta = 0.0f;
        impl.camera.phi = 0.0f;
    }

    if (ImGui::Button("Bottom")) {
        impl.camera.theta = 0.0f;
        impl.camera.phi = 3.0f * fpi2;
    }

    ImGui::NewLine();

    ImGui::SliderFloat("radius", &impl.camera.radius, 0.0f, 10.0f);
    ImGui::SliderFloat("theta", &impl.camera.theta, 0.0f, 2.0f * fpi);
    ImGui::SliderFloat("phi", &impl.camera.phi, 0.0f, 2.0f * fpi);
    ImGui::InputFloat("fov", &impl.camera.fov);
    ImGui::InputFloat("znear", &impl.camera.znear);
    ImGui::InputFloat("zfar", &impl.camera.zfar);
    ImGui::NewLine();
    ImGui::SliderFloat("pan_x", &impl.camera.focusPoint.x, -100.0f, 100.0f);
    ImGui::SliderFloat("pan_y", &impl.camera.focusPoint.y, -100.0f, 100.0f);
    ImGui::SliderFloat("pan_z", &impl.camera.focusPoint.z, -100.0f, 100.0f);

    ImGui::Separator();

    ImGui::SliderFloat("light_dir_x", &impl.lightDir.x, -1.0f, 1.0f);
    ImGui::SliderFloat("light_dir_y", &impl.lightDir.y, -1.0f, 1.0f);
    ImGui::SliderFloat("light_dir_z", &impl.lightDir.z, -1.0f, 1.0f);
    ImGui::ColorEdit3("light_color", reinterpret_cast<float*>(&impl.lightCol));
    ImGui::ColorEdit3("background color", reinterpret_cast<float*>(&impl.lightCol));

    ImGui::Separator();
}


static void drawMainMenuContents(osc::UiModelViewer::Impl& impl) {
    if (ImGui::BeginMenu("Options")) {
        drawOptionsMenu(impl);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scene")) {
        drawSceneMenu(impl);
        ImGui::EndMenu();
    }
}

static float computeRimColor(OpenSim::Component const* selected,
                             OpenSim::Component const* hovered,
                             OpenSim::Component const* c) {

    if (!c) {
        return 0.0f;
    }

    while (c) {
        if (c == selected) {
            return 1.0f;
        }
        if (c == hovered) {
            return 0.5f;
        }
        if (!c->hasOwner()) {
            return 0.0f;
        }
        c = &c->getOwner();
    }

    return 0.0f;
}

static void computeRimHighlights(osc::UiModelViewer::Impl& impl, RenderableScene const& rs) {
    auto decs = rs.getSceneDecorations();

    impl.rimHighlights.clear();
    impl.rimHighlights.reserve(decs.size());
    OpenSim::Component const* selected = rs.getSelected();
    OpenSim::Component const* hovered = rs.getHovered();
    for (size_t i = 0; i < decs.size(); ++i) {
        OpenSim::Component const* c = decs[i].component;
        impl.rimHighlights.push_back(computeRimColor(selected, hovered, c));
    }
}


UiModelViewerResponse osc::UiModelViewer::draw(RenderableScene const& rs) {
    // TODO: isolated components....

    Impl& impl = *m_Impl;

    // update camera if necessary
    if (impl.renderHovered) {
        UpdatePolarCameraFromImGuiUserInput(App::cur().dims(), impl.camera);
    }

    // draw main menu
    // draw panel menu
    if (ImGui::BeginMenuBar()) {
        drawMainMenuContents(impl);
        ImGui::EndMenuBar();
    }

    // put 3D scene in an undraggable child panel, to prevent accidental panel
    // dragging when the user drags their mouse over the scene
    if (!ImGui::BeginChild("##child", {0.0f, 0.0f}, false, ImGuiWindowFlags_NoMove)) {
        return {};  // child not visible
    }
    OSC_SCOPE_GUARD({ ImGui::EndChild(); });

    OpenSim::Component const* htResult = hittestSceneDecorations(impl, rs);
    computeRimHighlights(impl, rs);
    drawSceneTexture(impl, rs);
    drawOverlays(impl, rs);
    blitSceneTexture(impl);

    UiModelViewerResponse resp;
    resp.hovertestResult = htResult;
    resp.isMousedOver = impl.renderHovered;
    resp.isLeftClicked = impl.renderLeftClicked;
    resp.isRightClicked = impl.renderRightClicked;
    return resp;
}
