#include "UiModelViewer.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Shaders/EdgeDetectionShader.hpp"
#include "src/Graphics/Shaders/GouraudShader.hpp"
#include "src/Graphics/Shaders/InstancedGouraudColorShader.hpp"
#include "src/Graphics/Shaders/InstancedSolidColorShader.hpp"
#include "src/Graphics/Shaders/NormalsShader.hpp"
#include "src/Graphics/Shaders/SolidColorShader.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"
#include "src/Graphics/Texturing.hpp"
#include "src/OpenSimBindings/ComponentDecoration.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_events.h>
#include <IconsFontAwesome5.h>

#include <unordered_map>
#include <vector>

// helper method for making a render buffer (used in Render_target)
static gl::RenderBuffer makeMultisampledRenderBuffer(int samples, GLenum format, int w, int h)
{
    gl::RenderBuffer rv;
    gl::BindRenderBuffer(rv);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, w, h);
    return rv;
}

static gl::RenderBuffer makeRenderBuffer(GLenum format, int w, int h)
{
    gl::RenderBuffer rv;
    gl::BindRenderBuffer(rv);
    glRenderbufferStorage(GL_RENDERBUFFER, format, w, h);
    return rv;
}

namespace
{
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

    osc::PolarPerspectiveCamera CreateDefaultCamera()
    {
        osc::PolarPerspectiveCamera rv;
        rv.radius = 5.0f;
        return rv;
    }
}

struct osc::UiModelViewer::Impl final {
    UiModelViewerFlags flags;

    // used to cache between frames
    UID m_LastModelVersion;
    UID m_LastStateVersion;
    OpenSim::ComponentPath m_LastSelection;
    OpenSim::ComponentPath m_LastHover;
    OpenSim::ComponentPath m_LastIsolation;
    float m_LastFixupFactor;

    std::vector<osc::ComponentDecoration> m_Decorations;
    osc::BVH m_SceneBVH;
    PolarPerspectiveCamera camera = CreateDefaultCamera();
    glm::vec3 lightDir = {-0.34f, -0.25f, 0.05f};
    glm::vec3 lightCol = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    glm::vec4 backgroundCol = {0.89f, 0.89f, 0.89f, 1.0f};
    glm::vec4 rimCol = {1.0f, 0.4f, 0.0f, 0.85f};
    RenderBuffers renderTarg{{1, 1}, 1};

    // by default, lower the floor slightly, so that it doesn't conflict
    // with OpenSim ContactHalfSpace planes that coincidently happen to
    // lie at Z==0
    glm::vec3 floorLocation = {0.0f, -0.001f, 0.0f};

    gl::Texture2D chequerTex = genChequeredFloorTexture();

    std::vector<BVHCollision> sceneHittestResults;

    Rect renderRect{};

    bool renderHovered = false;
    bool renderLeftClicked = false;
    bool renderRightClicked = false;
    bool wireframeMode = false;
    bool drawMeshNormals = false;
    bool drawRims = true;
    bool autoFocusCameraNextFrame = false;

    std::vector<SceneGPUInstanceData> drawlistBuffer;

    GuiRuler ruler;

    Impl(UiModelViewerFlags flags_) : flags{flags_}
    {
        camera.theta = fpi4;
        camera.phi = fpi4;
    }
};

osc::UiModelViewer::UiModelViewer(UiModelViewerFlags flags) :
    m_Impl{new Impl{flags}}
{
}

osc::UiModelViewer::UiModelViewer(UiModelViewer&&) noexcept = default;

osc::UiModelViewer& osc::UiModelViewer::operator=(UiModelViewer&&) noexcept = default;

osc::UiModelViewer::~UiModelViewer() noexcept = default;

bool osc::UiModelViewer::isMousedOver() const
{
    return m_Impl->renderHovered;
}

static glm::mat4x3 GenerateFloorModelMatrix(osc::UiModelViewer::Impl const& impl,
                                            float fixupScaleFactor)
{
    // rotate from XY (+Z dir) to ZY (+Y dir)
    glm::mat4 rv = glm::rotate(glm::mat4{1.0f}, -osc::fpi2, {1.0f, 0.0f, 0.0f});

    // make floor extend far in all directions
    rv = glm::scale(glm::mat4{1.0f}, {fixupScaleFactor * 100.0f, 1.0f, fixupScaleFactor * 100.0f}) * rv;

    rv = glm::translate(glm::mat4{1.0f}, fixupScaleFactor * impl.floorLocation) * rv;

    return glm::mat4x3{rv};
}

static float ComputeRimColor(OpenSim::Component const* selected,
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

static bool IsInclusiveChildOf(OpenSim::Component const* parent, OpenSim::Component const* c)
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

static void PopulareSceneDrawlist(osc::UiModelViewer::Impl& impl,
                                  osc::VirtualConstModelStatePair const& msp)
{
    OpenSim::Component const* const selected = msp.getSelected();
    OpenSim::Component const* const hovered = msp.getHovered();
    OpenSim::Component const* const isolated = msp.getIsolated();

    if (msp.getModelVersion() != impl.m_LastModelVersion ||
        msp.getStateVersion() != impl.m_LastStateVersion ||
        selected != osc::FindComponent(msp.getModel(), impl.m_LastSelection) ||
        hovered != osc::FindComponent(msp.getModel(), impl.m_LastHover) ||
        isolated != osc::FindComponent(msp.getModel(), impl.m_LastHover) ||
        msp.getFixupScaleFactor() != impl.m_LastFixupFactor)
    {
        {
            OSC_PERF("generate decorations");
            osc::GenerateModelDecorations(msp, impl.m_Decorations);
        }

        {
            OSC_PERF("generate BVH");
            osc::UpdateSceneBVH(impl.m_Decorations, impl.m_SceneBVH);
        }

        impl.m_LastModelVersion = msp.getModelVersion();
        impl.m_LastStateVersion = msp.getStateVersion();
        impl.m_LastSelection = selected ? selected->getAbsolutePath() : OpenSim::ComponentPath{};
        impl.m_LastHover = hovered ? hovered->getAbsolutePath() : OpenSim::ComponentPath{};
        impl.m_LastIsolation = isolated ? isolated->getAbsolutePath() : OpenSim::ComponentPath{};
        impl.m_LastFixupFactor = msp.getFixupScaleFactor();
    }

    nonstd::span<osc::ComponentDecoration const> decs = impl.m_Decorations;

    // clear it (could've been populated by the last drawcall)
    std::vector<SceneGPUInstanceData>& buf = impl.drawlistBuffer;
    buf.clear();
    buf.reserve(decs.size());

    // populate the list with the scene
    for (size_t i = 0; i < decs.size(); ++i)
    {
        osc::ComponentDecoration const& se = decs[i];

        if (isolated && !IsInclusiveChildOf(isolated, se.component))
        {
            continue;  // skip rendering this (it's not in the isolated component)
        }

        SceneGPUInstanceData& ins = buf.emplace_back();
        ins.modelMtx = osc::ToMat4(se.transform);
        ins.normalMtx = osc::ToNormalMatrix(se.transform);
        ins.rgba = se.color;
        ins.rimIntensity = ComputeRimColor(selected, hovered, se.component);
        ins.decorationIdx = static_cast<int>(i);
    }
}

static void BindToInstanceAttributes(size_t offset)
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

static void DrawSceneTexture(osc::UiModelViewer::Impl& impl, float fixupScaleFactor)
{
    auto& renderTarg = impl.renderTarg;

    // ensure buffer sizes match ImGui panel size
    {
        ImVec2 contentRegion = ImGui::GetContentRegionAvail();
        if (contentRegion.x >= 1.0f && contentRegion.y >= 1.0f)
        {
            glm::ivec2 dims{static_cast<int>(contentRegion.x), static_cast<int>(contentRegion.y)};
            renderTarg.setDims(dims);
            renderTarg.setSamples(osc::App::cur().getMSXAASamplesRecommended());
        }
    }

    // instance data to the GPU
    gl::ArrayBuffer<SceneGPUInstanceData> instanceBuf{impl.drawlistBuffer};

    // get scene matrices
    glm::mat4 projMtx = impl.camera.getProjMtx(static_cast<float>(renderTarg.dims.x)/static_cast<float>(renderTarg.dims.y));
    glm::mat4 viewMtx = impl.camera.getViewMtx();
    glm::vec3 viewerPos = impl.camera.getPos();

    // setup top-level OpenGL state
    gl::Viewport(0, 0, renderTarg.dims.x, renderTarg.dims.y);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl::Enable(GL_BLEND);
    gl::Enable(GL_DEPTH_TEST);
    gl::Disable(GL_SCISSOR_TEST);

    // draw scene
    {
        gl::BindFramebuffer(GL_FRAMEBUFFER, renderTarg.sceneFBO);
        gl::ClearColor(impl.backgroundCol);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (impl.wireframeMode)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        auto& instancedShader = osc::App::shader<osc::InstancedGouraudColorShader>();
        gl::UseProgram(instancedShader.program);
        gl::Uniform(instancedShader.uProjMat, projMtx);
        gl::Uniform(instancedShader.uViewMat, viewMtx);
        gl::Uniform(instancedShader.uLightDir, impl.lightDir);
        gl::Uniform(instancedShader.uLightColor, impl.lightCol);
        gl::Uniform(instancedShader.uViewPos, viewerPos);

        std::vector<SceneGPUInstanceData> const& instances = impl.drawlistBuffer;
        nonstd::span<osc::ComponentDecoration const> decs = impl.m_Decorations;

        size_t pos = 0;
        size_t ninstances = instances.size();

        while (pos < ninstances)
        {
            osc::ComponentDecoration const& se = decs[instances[pos].decorationIdx];

            // batch
            size_t end = pos + 1;
            while (end < ninstances &&
                   decs[instances[end].decorationIdx].mesh.get() == se.mesh.get())
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

        if (impl.flags & osc::UiModelViewerFlags_DrawFloor)
        {
            auto& basicShader = osc::App::shader<osc::GouraudShader>();

            gl::UseProgram(basicShader.program);
            gl::Uniform(basicShader.uProjMat, projMtx);
            gl::Uniform(basicShader.uViewMat, viewMtx);
            glm::mat4 mtx = GenerateFloorModelMatrix(impl, fixupScaleFactor);
            gl::Uniform(basicShader.uModelMat, mtx);
            gl::Uniform(basicShader.uNormalMat, osc::ToNormalMatrix(mtx));
            gl::Uniform(basicShader.uLightDir, impl.lightDir);
            gl::Uniform(basicShader.uLightColor, impl.lightCol);
            gl::Uniform(basicShader.uViewPos, viewerPos);
            gl::Uniform(basicShader.uDiffuseColor, {1.0f, 1.0f, 1.0f, 1.0f});
            gl::Uniform(basicShader.uIsTextured, true);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(impl.chequerTex);
            gl::Uniform(basicShader.uSampler0, gl::textureIndex<GL_TEXTURE0>());
            auto floor = osc::App::meshes().getFloorMesh();
            gl::BindVertexArray(floor->GetVertexArray());
            floor->Draw();
            gl::BindVertexArray();
        }

        if (impl.wireframeMode)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    // draw mesh normals, if requested
    if (impl.drawMeshNormals)
    {
        auto& normalShader = osc::App::shader<osc::NormalsShader>();
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::UseProgram(normalShader.program);
        gl::Uniform(normalShader.uProjMat, projMtx);
        gl::Uniform(normalShader.uViewMat, viewMtx);

        std::vector<SceneGPUInstanceData> const& instances = impl.drawlistBuffer;
        nonstd::span<osc::ComponentDecoration const> decs = impl.m_Decorations;

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
    gl::BindFramebuffer(GL_READ_FRAMEBUFFER, renderTarg.sceneFBO);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, renderTarg.outputFbo);
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::BlitFramebuffer(0, 0, renderTarg.dims.x, renderTarg.dims.y,
                        0, 0, renderTarg.dims.x, renderTarg.dims.y,
                        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    // draw rims
    if (impl.drawRims)
    {
        gl::BindFramebuffer(GL_FRAMEBUFFER, renderTarg.rimsFBO);
        gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto& iscs = osc::App::shader<osc::InstancedSolidColorShader>();
        gl::UseProgram(iscs.program);
        gl::Uniform(iscs.uVP, projMtx * viewMtx);

        std::vector<SceneGPUInstanceData> const& instances = impl.drawlistBuffer;
        nonstd::span<osc::ComponentDecoration const> decs = impl.m_Decorations;

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
                   decs[instances[end].decorationIdx].mesh.get() == se.mesh.get() &&
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
                rimAABB = Union(rimAABB, decs[instances[i].decorationIdx].worldspaceAABB);
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
            float rimThickness = 1.0f / std::min(renderTarg.dims.x, renderTarg.dims.y);

            // care: edge-detection kernels are expensive
            //
            // calculate a screenspace bounding box that surrounds the rims so that the
            // edge detection shader only had to run on a smaller subset of the screen
            osc::AABB screenspaceRimBounds = TransformAABB(rimAABB, projMtx * viewMtx);
            auto verts = ToCubeVerts(screenspaceRimBounds);
            glm::vec2 bounds[2] = {{verts[0].x, verts[0].y}, {verts[0].x, verts[0].y}};
            for (size_t i = 1; i < verts.size(); ++i)
            {
                glm::vec2 p{verts[i].x, verts[i].y};
                bounds[0] = osc::Min(p, bounds[0]);
                bounds[1] = osc::Max(p, bounds[1]);
            }
            bounds[0] -= rimThickness;
            bounds[1] += rimThickness;
            glm::ivec2 renderDims = renderTarg.dims;
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

            // rims FBO now contains *solid* colors that need to be edge-detected

            // write rims over the output output
            gl::BindFramebuffer(GL_FRAMEBUFFER, renderTarg.outputFbo);

            auto& edgeDetectShader = osc::App::shader<osc::EdgeDetectionShader>();
            gl::UseProgram(edgeDetectShader.program);
            gl::Uniform(edgeDetectShader.uMVP, gl::identity);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(renderTarg.rims2DTex);
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

static void BlitSceneTexture(osc::UiModelViewer::Impl& impl)
{
    ImVec2 imgDims = ImGui::GetContentRegionAvail();
    osc::DrawTextureAsImGuiImage(impl.renderTarg.outputTex, imgDims);

    impl.renderRect.p1 = ImGui::GetItemRectMin();
    impl.renderRect.p2 = ImGui::GetItemRectMax();
    impl.renderHovered = ImGui::IsItemHovered();
    impl.renderLeftClicked = ImGui::IsItemHovered() && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
    impl.renderRightClicked = ImGui::IsItemHovered() && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right);
}

static std::pair<OpenSim::Component const*, glm::vec3> HittestDecorations(
    osc::UiModelViewer::Impl& impl,
    osc::VirtualConstModelStatePair const& msp)
{
    if (!impl.renderHovered)
    {
        return {nullptr, {0.0f, 0.0f, 0.0f}};
    }

    // figure out mouse pos in panel's NDC system
    glm::vec2 windowScreenPos = ImGui::GetWindowPos();  // where current ImGui window is in the screen
    glm::vec2 mouseScreenPos = ImGui::GetMousePos();  // where mouse is in the screen
    glm::vec2 mouseWindowPos = mouseScreenPos - windowScreenPos;  // where mouse is in current window
    glm::vec2 cursorWindowPos = ImGui::GetCursorPos();  // where cursor is in current window
    glm::vec2 mouseItemPos = mouseWindowPos - cursorWindowPos;  // where mouse is in current item
    glm::vec2 itemDims = ImGui::GetContentRegionAvail();  // how big current window will be

    // un-project the mouse position as a ray in worldspace
    osc::Line cameraRay = impl.camera.unprojectTopLeftPosToWorldRay(mouseItemPos, itemDims);

    // use scene BVH to intersect that ray with the scene
    impl.sceneHittestResults.clear();
    BVH_GetRayAABBCollisions(impl.m_SceneBVH, cameraRay, &impl.sceneHittestResults);

    // go through triangle BVHes to figure out which, if any, triangle is closest intersecting
    int closestIdx = -1;
    float closestDistance = std::numeric_limits<float>::max();
    glm::vec3 closestWorldLoc = {0.0f, 0.0f, 0.0f};

    // iterate through each scene-level hit and perform a triangle-level hittest
    nonstd::span<osc::ComponentDecoration const> decs = impl.m_Decorations;
    OpenSim::Component const* const isolated = msp.getIsolated();

    for (osc::BVHCollision const& c : impl.sceneHittestResults)
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
        return {decs[closestIdx].component,  closestWorldLoc};
    }
    else
    {
        return {nullptr, {0.0f, 0.0f, 0.0f}};
    }
}

static void DrawGrid(osc::UiModelViewer::Impl& impl, glm::mat4 rotationMatrix)
{
    auto& shader = osc::App::shader<osc::SolidColorShader>();

    gl::UseProgram(shader.program);
    gl::Uniform(shader.uModel, glm::scale(rotationMatrix, {50.0f, 50.0f, 1.0f}));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(AspectRatio(impl.renderRect)));
    gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
    auto grid = osc::App::meshes().get100x100GridMesh();
    gl::BindVertexArray(grid->GetVertexArray());
    grid->Draw();
    gl::BindVertexArray();
}

static void DrawXZGrid(osc::UiModelViewer::Impl& impl)
{
    DrawGrid(impl, glm::rotate(glm::mat4{1.0f}, osc::fpi2, {1.0f, 0.0f, 0.0f}));
}

static void DrawXYGrid(osc::UiModelViewer::Impl& impl)
{
    DrawGrid(impl, glm::mat4{1.0f});
}

static void DrawYZGrid(osc::UiModelViewer::Impl& impl)
{
    DrawGrid(impl, glm::rotate(glm::mat4{1.0f}, osc::fpi2, {0.0f, 1.0f, 0.0f}));
}

static void DrawXZFloorLines(osc::UiModelViewer::Impl& impl)
{
    auto& shader = osc::App::shader<osc::SolidColorShader>();

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(AspectRatio(impl.renderRect)));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());

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

static void DrawAABBs(osc::UiModelViewer::Impl& impl)
{
    auto& shader = osc::App::shader<osc::SolidColorShader>();

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(AspectRatio(impl.renderRect)));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

    auto cube = osc::App::meshes().getCubeWireMesh();
    gl::BindVertexArray(cube->GetVertexArray());

    for (auto const& se : impl.m_Decorations)
    {
        glm::vec3 halfWidths = Dimensions(se.worldspaceAABB) / 2.0f;
        glm::vec3 center = Midpoint(se.worldspaceAABB);

        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
        glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
        glm::mat4 mmtx = mover * scaler;

        gl::Uniform(shader.uModel, mmtx);
        cube->Draw();
    }

    gl::BindVertexArray();
}

// assumes `pos` is in-bounds
static void DrawBVHRecursive(osc::Mesh& cube,
                             gl::UniformMat4& mtxUniform,
                             osc::BVH const& bvh, int pos)
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

static void DrawBVH(osc::UiModelViewer::Impl& impl)
{
    if (impl.m_SceneBVH.nodes.empty())
    {
        return;
    }

    auto& shader = osc::App::shader<osc::SolidColorShader>();

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(AspectRatio(impl.renderRect)));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

    auto cube = osc::App::meshes().getCubeWireMesh();
    gl::BindVertexArray(cube->GetVertexArray());
    DrawBVHRecursive(*cube, shader.uModel, impl.m_SceneBVH, 0);
    gl::BindVertexArray();
}

// draws overlays that are "in scene" - i.e. they are part of the rendered texture
static void DrawInSceneOverlays(osc::UiModelViewer::Impl& impl)
{
    gl::BindFramebuffer(GL_FRAMEBUFFER, impl.renderTarg.outputFbo);
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);

    if (impl.flags & osc::UiModelViewerFlags_DrawXZGrid)
    {
        DrawXZGrid(impl);
    }

    if (impl.flags & osc::UiModelViewerFlags_DrawXYGrid)
    {
        DrawXYGrid(impl);
    }

    if (impl.flags & osc::UiModelViewerFlags_DrawYZGrid)
    {
        DrawYZGrid(impl);
    }

    if (impl.flags & osc::UiModelViewerFlags_DrawAxisLines)
    {
        DrawXZFloorLines(impl);
    }

    if (impl.flags & osc::UiModelViewerFlags_DrawAABBs)
    {
        DrawAABBs(impl);
    }

    if (impl.flags & osc::UiModelViewerFlags_DrawBVH)
    {
        DrawBVH(impl);
    }

    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
}

// draw 2D overlays that are emitted via imgui (rather than inside the rendered texture)
static void DrawImGuiOverlays(osc::UiModelViewer::Impl& impl)
{
    if (impl.flags & osc::UiModelViewerFlags_DrawAlignmentAxes)
    {
        DrawAlignmentAxesOverlayInBottomRightOf(impl.camera.getViewMtx(), impl.renderRect);
    }
}

static void DrawOptionsMenuContent(osc::UiModelViewer::Impl& impl)
{
    ImGui::Checkbox("wireframe mode", &impl.wireframeMode);
    ImGui::Checkbox("show normals", &impl.drawMeshNormals);
    ImGui::Checkbox("draw rims", &impl.drawRims);
    ImGui::CheckboxFlags("show XZ grid", &impl.flags, osc::UiModelViewerFlags_DrawXZGrid);
    ImGui::CheckboxFlags("show XY grid", &impl.flags, osc::UiModelViewerFlags_DrawXYGrid);
    ImGui::CheckboxFlags("show YZ grid", &impl.flags, osc::UiModelViewerFlags_DrawYZGrid);
    ImGui::CheckboxFlags("show alignment axes", &impl.flags, osc::UiModelViewerFlags_DrawAlignmentAxes);
    ImGui::CheckboxFlags("show grid lines", &impl.flags, osc::UiModelViewerFlags_DrawAxisLines);
    ImGui::CheckboxFlags("show AABBs", &impl.flags, osc::UiModelViewerFlags_DrawAABBs);
    ImGui::CheckboxFlags("show BVH", &impl.flags, osc::UiModelViewerFlags_DrawBVH);
    ImGui::CheckboxFlags("show floor", &impl.flags, osc::UiModelViewerFlags_DrawFloor);
}

static void DoFocusCameraAlongX(osc::UiModelViewer::Impl& impl)
{
    impl.camera.theta = osc::fpi2;
    impl.camera.phi = 0.0f;
}

static void DoFocusCameraAlongMinusX(osc::UiModelViewer::Impl& impl)
{
    impl.camera.theta = -osc::fpi2;
    impl.camera.phi = 0.0f;
}

static void DoFocusCameraAlongY(osc::UiModelViewer::Impl& impl)
{
    impl.camera.theta = 0.0f;
    impl.camera.phi = osc::fpi2;
}

static void DoFocusCameraAlongMinusY(osc::UiModelViewer::Impl& impl)
{
    impl.camera.theta = 0.0f;
    impl.camera.phi = -osc::fpi2;
}

static void DoFocusCameraAlongZ(osc::UiModelViewer::Impl& impl)
{
    impl.camera.theta = 0.0f;
    impl.camera.phi = 0.0f;
}

static void DoFocusCameraAlongMinusZ(osc::UiModelViewer::Impl& impl)
{
    impl.camera.theta = osc::fpi;
    impl.camera.phi = 0.0f;
}

static void DoResetCamera(osc::UiModelViewer::Impl& impl)
{
    impl.camera = {};
    impl.camera.theta = osc::fpi4;
    impl.camera.phi = osc::fpi4;
}

static void DoAutoFocusCamera(osc::UiModelViewer::Impl& impl)
{
    if (!impl.m_SceneBVH.nodes.empty())
    {
        auto const& bvhRoot = impl.m_SceneBVH.nodes[0].bounds;
        impl.camera.focusPoint = -Midpoint(bvhRoot);
        impl.camera.radius = 2.0f * LongestDim(bvhRoot);
        impl.camera.theta = osc::fpi4;
        impl.camera.phi = osc::fpi4;
    }
}

static void DrawSceneMenu(osc::UiModelViewer::Impl& impl)
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
        DoFocusCameraAlongX(impl);
    }
    makeHoverTooltip("Position camera along +X, pointing towards the center. Hotkey: X");
    ImGui::SameLine();
    if (ImGui::Button("-X"))
    {
        DoFocusCameraAlongMinusX(impl);
    }
    makeHoverTooltip("Position camera along -X, pointing towards the center. Hotkey: Ctrl+X");

    ImGui::SameLine();
    if (ImGui::Button("+Y"))
    {
        DoFocusCameraAlongY(impl);
    }
    makeHoverTooltip("Position camera along +Y, pointing towards the center. Hotkey: Y");
    ImGui::SameLine();
    if (ImGui::Button("-Y"))
    {
        DoFocusCameraAlongMinusY(impl);
    }
    makeHoverTooltip("Position camera along -Y, pointing towards the center. (no hotkey, because Ctrl+Y is taken by 'Redo'");

    ImGui::SameLine();
    if (ImGui::Button("+Z"))
    {
        DoFocusCameraAlongZ(impl);
    }
    makeHoverTooltip("Position camera along +Z, pointing towards the center. Hotkey: Z");
    ImGui::SameLine();
    if (ImGui::Button("-Z"))
    {
        DoFocusCameraAlongMinusZ(impl);
    }
    makeHoverTooltip("Position camera along -Z, pointing towards the center. (no hotkey, because Ctrl+Z is taken by 'Undo')");

    if (ImGui::Button("Zoom in"))
    {
        impl.camera.radius *= 0.8f;
    }

    ImGui::SameLine();
    if (ImGui::Button("Zoom out"))
    {
        impl.camera.radius *= 1.2f;
    }

    if (ImGui::Button("reset camera"))
    {
        DoResetCamera(impl);
    }
    makeHoverTooltip("Reset the camera to its initial (default) location. Hotkey: F");

    if (ImGui::Button("Auto-focus camera"))
    {
        impl.autoFocusCameraNextFrame = true;
    }
    makeHoverTooltip("Try to automatically adjust the camera's zoom etc. to suit the model's dimensions. Hotkey: Ctrl+F");

    ImGui::Dummy({0.0f, 10.0f});
    ImGui::Text("advanced camera properties:");
    ImGui::Separator();
    osc::SliderMetersFloat("radius", &impl.camera.radius, 0.0f, 10.0f);
    ImGui::SliderFloat("theta", &impl.camera.theta, 0.0f, 2.0f * osc::fpi);
    ImGui::SliderFloat("phi", &impl.camera.phi, 0.0f, 2.0f * osc::fpi);
    ImGui::InputFloat("fov", &impl.camera.fov);
    osc::InputMetersFloat("znear", &impl.camera.znear);
    osc::InputMetersFloat("zfar", &impl.camera.zfar);
    ImGui::NewLine();
    osc::SliderMetersFloat("pan_x", &impl.camera.focusPoint.x, -100.0f, 100.0f);
    osc::SliderMetersFloat("pan_y", &impl.camera.focusPoint.y, -100.0f, 100.0f);
    osc::SliderMetersFloat("pan_z", &impl.camera.focusPoint.z, -100.0f, 100.0f);

    ImGui::Dummy({0.0f, 10.0f});
    ImGui::Text("advanced scene properties:");
    ImGui::Separator();
    ImGui::ColorEdit3("light_color", reinterpret_cast<float*>(&impl.lightCol));
    ImGui::ColorEdit3("background color", reinterpret_cast<float*>(&impl.backgroundCol));
    osc::InputMetersFloat3("floor location", &impl.floorLocation.x);
    makeHoverTooltip("Set the origin location of the scene's chequered floor. This is handy if you are working on smaller models, or models that need a floor somewhere else");
}


static void DrawMainMenuContent(osc::UiModelViewer::Impl& impl)
{
    if (ImGui::BeginMenu("Options"))
    {
        DrawOptionsMenuContent(impl);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scene"))
    {
        DrawSceneMenu(impl);
        ImGui::EndMenu();
    }

    if (impl.ruler.IsMeasuring())
    {
        if (ImGui::MenuItem(ICON_FA_RULER " measuring", nullptr, false, false))
        {
            impl.ruler.StopMeasuring();
        }
    }
    else
    {
        if (ImGui::MenuItem(ICON_FA_RULER " measure", nullptr, false, true))
        {
            impl.ruler.StartMeasuring();
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("EXPERIMENTAL: take a *rough* measurement of something in the scene - the UI for this needs to be improved, a lot ;)");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

void osc::UiModelViewer::requestAutoFocus()
{
    m_Impl->autoFocusCameraNextFrame = true;
}

osc::UiModelViewerResponse osc::UiModelViewer::draw(VirtualConstModelStatePair const& rs)
{
    Impl& impl = *m_Impl;

    // update camera if necessary
    if (impl.renderHovered)
    {
        bool ctrlDown = osc::IsCtrlOrSuperDown();

        if (ImGui::IsKeyReleased(SDL_SCANCODE_X))
        {
            if (ctrlDown)
            {
                DoFocusCameraAlongMinusX(impl);
            } else
            {
                DoFocusCameraAlongX(impl);
            }
        }
        if (ImGui::IsKeyPressed(SDL_SCANCODE_Y))
        {
            if (!ctrlDown)
            {
                DoFocusCameraAlongY(impl);
            }
        }
        if (ImGui::IsKeyPressed(SDL_SCANCODE_Z))
        {
            if (!ctrlDown)
            {
                DoFocusCameraAlongZ(impl);
            }
        }
        if (ImGui::IsKeyPressed(SDL_SCANCODE_F))
        {
            if (ctrlDown)
            {
                impl.autoFocusCameraNextFrame = true;
            }
            else
            {
                DoResetCamera(impl);
            }
        }
        if (ctrlDown && (ImGui::IsKeyPressed(SDL_SCANCODE_8)))
        {
            // solidworks keybind
            impl.autoFocusCameraNextFrame = true;
        }
        UpdatePolarCameraFromImGuiUserInput(App::cur().dims(), impl.camera);
    }

    // draw main menu
    if (ImGui::BeginMenuBar())
    {
        DrawMainMenuContent(impl);
        ImGui::EndMenuBar();
    }

    // automatically change lighting position based on camera position
    {
        glm::vec3 p = glm::normalize(-impl.camera.focusPoint - impl.camera.getPos());
        glm::vec3 up = {0.0f, 1.0f, 0.0f};
        glm::vec3 mp = glm::rotate(glm::mat4{1.0f}, 1.25f * fpi4, up) * glm::vec4{p, 0.0f};
        impl.lightDir = glm::normalize(mp + -up);
    }

    // put 3D scene in an undraggable child panel, to prevent accidental panel
    // dragging when the user drags their mouse over the scene
    if (ImGui::BeginChild("##child", {0.0f, 0.0f}, false, ImGuiWindowFlags_NoMove))
    {
        // only do the hit test if the user isn't currently dragging their mouse around
        std::pair<OpenSim::Component const*, glm::vec3> htResult{nullptr, {0.0f, 0.0f, 0.0f}};
        if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
            !ImGui::IsMouseDragging(ImGuiMouseButton_Middle) &&
            !ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            htResult = HittestDecorations(impl, rs);
        }

        PopulareSceneDrawlist(impl, rs);

        // auto-focus the camera, if the user requested it last frame
        //
        // care: indirectly depends on the scene drawlist being up-to-date
        if (impl.autoFocusCameraNextFrame)
        {
            DoAutoFocusCamera(impl);
            impl.autoFocusCameraNextFrame = false;
        }

        DrawSceneTexture(impl, rs.getFixupScaleFactor());
        DrawInSceneOverlays(impl);
        BlitSceneTexture(impl);
        DrawImGuiOverlays(impl);

        if (impl.ruler.IsMeasuring())
        {
            impl.ruler.draw(htResult, impl.camera, impl.renderRect);
        }

        ImGui::EndChild();

        if (impl.ruler.IsMeasuring())
        {
            return {};  // don't give the caller the hittest result while measuring
        }
        else
        {
            UiModelViewerResponse resp;
            resp.hovertestResult = htResult.first;
            resp.isMousedOver = impl.renderHovered;
            if (resp.isMousedOver) {
                resp.mouse3DLocation = htResult.second;
            }
            resp.isLeftClicked = impl.renderLeftClicked;
            resp.isRightClicked = impl.renderRightClicked;
            return resp;
        }
    }
    else
    {
        return {};  // child not visible
    }
}
