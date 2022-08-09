#include "RendererOpenSimTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/TextureGen.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Widgets/LogViewerPanel.hpp"
#include "src/Widgets/PerfPanel.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_events.h>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    struct NewDecoration final {
        std::shared_ptr<osc::experimental::Mesh const> Mesh;
        osc::Transform Transform;
        glm::vec4 Color;
        bool IsHovered;

        NewDecoration(osc::SceneDecoration const& d) :
            Mesh{d.mesh},
            Transform{d.transform},
            Color{d.color},
            IsHovered{static_cast<bool>(d.flags & osc::SceneDecorationFlags_IsHovered)}
        {
        }
    };

    osc::AABB WorldpaceAABB(NewDecoration const& d)
    {
        return osc::TransformAABB(d.Mesh->getBounds(), d.Transform);
    }

    std::vector<NewDecoration> GenerateDecorations()
    {
        osc::UndoableModelStatePair p{std::make_unique<OpenSim::Model>(osc::App::resource("models/RajagopalModel/Rajagopal2015.osim").string())};
        std::vector<osc::SceneDecoration> decs;
        osc::GenerateModelDecorations(p, decs);
        std::vector<NewDecoration> rv;
        rv.reserve(decs.size());
        for (osc::SceneDecoration const& dec : decs)
        {
            auto& d = rv.emplace_back(dec);
            if (osc::ContainsSubstring(dec.id, "torso_geom_4"))
            {
                d.IsHovered = true;
            }
        }
        return rv;
    }

    osc::Transform GetFloorTransform()
    {
        osc::Transform rv;
        rv.rotation = glm::angleAxis(-osc::fpi2, glm::vec3{1.0f, 0.0f, 0.0f});
        rv.scale = {100.0f, 100.0f, 1.0f};
        return rv;
    }
}

class osc::RendererOpenSimTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{parent}
    {
        m_FloorTexture.setFilterMode(experimental::TextureFilterMode::Mipmap);

        m_SolidColorMaterial.setVec4("uDiffuseColor", {1.0f, 0.0f, 0.0f, 1.0f});
        m_SceneTexturedElementsMaterial.setTexture("uDiffuseTexture", m_FloorTexture);
        m_SceneTexturedElementsMaterial.setVec2("uTextureScale", {200.0f, 200.0f});

        m_LogPanel.open();
        m_PerfPanel.open();
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return "Renderer (OpenSim)";
    }

    TabHost* getParent() const
    {
        return m_Parent;
    }

    void onMount()
    {
    }

    void onUnmount()
    {
    }

    bool onEvent(SDL_Event const& e)
    {
        return false;
    }

    void onTick()
    {
    }

    void onDrawMainMenu()
    {
    }

    void onDraw()
    {
        // compute viewport rect+dimensions (the render fills the viewport)
        Rect const viewportRect = osc::GetMainViewportWorkspaceScreenRect();
        glm::vec2 const viewportRectDims = osc::Dimensions(viewportRect);

        // configure scene texture to match viewport dimensions
        experimental::RenderTextureDescriptor sceneTextureDescriptor
        {
            static_cast<int>(viewportRectDims.x),
            static_cast<int>(viewportRectDims.y),
        };
        sceneTextureDescriptor.setAntialiasingLevel(osc::App::get().getMSXAASamplesRecommended());
        EmplaceOrReformat(m_SceneTex, sceneTextureDescriptor);

        // update the (purely mathematical) polar camera from user input
        osc::UpdatePolarCameraFromImGuiUserInput(viewportRectDims, m_PolarCamera);
        glm::vec3 const lightDir = RecommendedLightDirection(m_PolarCamera);

        // update the (rendered to) scene camera
        m_Camera.setPosition(m_PolarCamera.getPos());
        m_Camera.setNearClippingPlane(m_PolarCamera.znear);
        m_Camera.setFarClippingPlane(m_PolarCamera.zfar);
        m_Camera.setViewMatrix(m_PolarCamera.getViewMtx());
        m_Camera.setProjectionMatrix(m_PolarCamera.getProjMtx(AspectRatio(viewportRectDims)));

        // these variables are updated if rims are in the render
        //
        // they're used as an optimization: the edge-detection algorithm ideally only
        // highlights where the rims are, rather than the whole screen
        bool hasRims = false;
        glm::mat4 ndcToRimsMat{1.0f};
        glm::vec2 rimOffsets = {0.0f, 0.0f};  // in "rim space"

        if (m_DrawRims)
        {
            // compute the worldspace bounds of the rim-highlighted geometry
            AABB rimAABBWorldspace = InvertedAABB();
            for (NewDecoration const& dec : m_Decorations)
            {
                if (dec.IsHovered)
                {
                    rimAABBWorldspace = Union(rimAABBWorldspace, WorldpaceAABB(dec));
                    hasRims = true;
                }
            }

            // figure out if the rims actually appear on the screen and (roughly) where
            std::optional<Rect> maybeRimRectNDC = hasRims ?
                AABBToScreenNDCRect(
                    rimAABBWorldspace,
                    m_Camera.getViewMatrix(),
                    m_Camera.getProjectionMatrix(),
                    m_Camera.getNearClippingPlane(),
                    m_Camera.getFarClippingPlane()) :
                std::nullopt;

            hasRims = maybeRimRectNDC.has_value();

            // if the scene has rims, and they can be projected onto the screen, then
            // render the rim geometry as a solid color and precompute the relevant
            // texture coordinates, quad transform, etc. for the (later) edge-detection
            // step
            if (hasRims)
            {
                Rect& rimRectNDC = *maybeRimRectNDC;

                glm::vec2 const rimThicknessNDC = 2.0f*glm::vec2{m_RimThickness} / glm::vec2{viewportRectDims};

                // expand by the rim thickness, so that the output has space for the rims
                rimRectNDC = osc::Expand(rimRectNDC, rimThicknessNDC);

                // constrain the result of the above maths to within clip space
                rimRectNDC.p1 = glm::max(rimRectNDC.p1, glm::vec2{-1.0f, -1.0f});
                rimRectNDC.p2 = glm::min(rimRectNDC.p2, glm::vec2{1.0f, 1.0f});

                // calculate rim rect in screenspace (pixels)
                Rect const rimRectScreen = NdcRectToScreenspaceViewportRect(rimRectNDC, Rect{{}, viewportRectDims});

                // calculate the dimensions of the rims in both NDC- and screen-space
                glm::vec2 const rimDimsNDC = osc::Dimensions(rimRectNDC);
                glm::vec2 const rimDimsScreen = osc::Dimensions(rimRectScreen);

                // resize the output texture (pixel) dimensions to match the (expanded) bounding rect
                experimental::RenderTextureDescriptor selectedDesc{sceneTextureDescriptor};
                selectedDesc.setWidth(static_cast<int>(rimDimsScreen.x));
                selectedDesc.setHeight(static_cast<int>(rimDimsScreen.y));
                selectedDesc.setColorFormat(experimental::RenderTextureFormat::RED);
                EmplaceOrReformat(m_SelectedTex, selectedDesc);

                // calculate a transform matrix that maps the bounding rect to the edges of clipspace
                //
                // this is so that the solid geometry render fills the output texture perfectly

                // scale output width (dims) to clipspace width (2.0f)
                glm::vec2 const scale = 2.0f / rimDimsNDC;

                // move output location to the edge of clipspace
                glm::vec2 const bottomLeftNDC = {-1.0f, -1.0f};
                glm::vec2 const position = bottomLeftNDC - (scale * glm::vec2{ glm::min(rimRectNDC.p1.x, rimRectNDC.p2.x), glm::min(rimRectNDC.p1.y, rimRectNDC.p2.y) });

                // compute transforms of the above
                Transform rimsToNDCtransform;
                rimsToNDCtransform.scale = {scale, 1.0f};
                rimsToNDCtransform.position = {position, 0.0f};

                // create matrices that render the rims to the edges of the output texture, and an inverse
                // matrix that maps this clipspace-sized quad back into its original dimensions
                glm::mat4 const rimsToNDCmat = ToMat4(rimsToNDCtransform);
                ndcToRimsMat = ToInverseMat4(rimsToNDCtransform);
                rimOffsets = glm::vec2{m_RimThickness} / rimDimsScreen;

                // draw the solid geometry that was stretched over clipspace
                for (NewDecoration const& dec : m_Decorations)
                {
                    if (dec.IsHovered)
                    {
                        experimental::Graphics::DrawMesh(*dec.Mesh, dec.Transform, m_SolidColorMaterial, m_Camera);
                    }
                }
                glm::mat4 const originalProjMat = m_Camera.getProjectionMatrix();
                m_Camera.setProjectionMatrix(rimsToNDCmat * originalProjMat);
                m_Camera.setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});
                m_Camera.swapTexture(m_SelectedTex);
                m_Camera.render();
                m_Camera.swapTexture(m_SelectedTex);
                m_Camera.setBackgroundColor(m_SceneBgColor);
                m_Camera.setProjectionMatrix(originalProjMat);
            }
        }

        // render scene to the screen
        {
            // draw OpenSim scene elements
            m_SceneColoredElementsMaterial.setVec3("uViewPos", m_PolarCamera.getPos());
            m_SceneColoredElementsMaterial.setVec3("uLightDir", lightDir);
            m_SceneColoredElementsMaterial.setVec3("uLightColor", m_LightColor);

            experimental::MaterialPropertyBlock propBlock;
            glm::vec4 lastColor = {-1.0f, -1.0f, -1.0f, 0.0f};
            for (NewDecoration const& dec : m_Decorations)
            {
                if (dec.Color != lastColor)
                {
                    propBlock.setVec4("uDiffuseColor", dec.Color);
                    lastColor = dec.Color;
                }

                experimental::Graphics::DrawMesh(*dec.Mesh, dec.Transform, m_SceneColoredElementsMaterial, m_Camera, propBlock);

                // if normals are requested, render the scene element via a normals geometry shader
                if (m_DrawNormals)
                {
                    experimental::Graphics::DrawMesh(*dec.Mesh, dec.Transform, m_NormalsMaterial, m_Camera);
                }
            }

            // if a floor is requested, draw a textured floor
            if (m_DrawFloor)
            {
                m_SceneTexturedElementsMaterial.setVec3("uViewPos", m_PolarCamera.getPos());
                m_SceneTexturedElementsMaterial.setVec3("uLightDir", lightDir);
                m_SceneTexturedElementsMaterial.setVec3("uLightColor", m_LightColor);
                m_SceneTexturedElementsMaterial.setFloat("uNear", m_Camera.getNearClippingPlane());
                m_SceneTexturedElementsMaterial.setFloat("uFar", m_Camera.getFarClippingPlane());
                m_SceneTexturedElementsMaterial.setTransparent(true);  // fog

                experimental::Graphics::DrawMesh(m_QuadMesh, m_FloorTransform, m_SceneTexturedElementsMaterial, m_Camera);
            }

            // if rims are requested, draw them
            if (hasRims)
            {
                m_EdgeDetectorMaterial.setRenderTexture("uScreenTexture", *m_SelectedTex);
                m_EdgeDetectorMaterial.setVec4("uRimRgba", m_RimRgba);
                m_EdgeDetectorMaterial.setVec2("uRimThickness", rimOffsets);
                m_EdgeDetectorMaterial.setTransparent(true);
                m_EdgeDetectorMaterial.setDepthTested(false);

                experimental::Graphics::DrawMesh(m_QuadMesh, m_Camera.getInverseViewProjectionMatrix() * ndcToRimsMat, m_EdgeDetectorMaterial, m_Camera);

                m_EdgeDetectorMaterial.clearRenderTexture("uScreenTexture");  // prevents copies on next frame
            }

            m_Camera.swapTexture(m_SceneTex);
            m_Camera.render();
            m_Camera.swapTexture(m_SceneTex);
        }

        experimental::Graphics::BlitToScreen(*m_SceneTex, viewportRect);

        // render auxiliary 2D UI
        {
            ImGui::Begin("controls");
            ImGui::Checkbox("draw normals", &m_DrawNormals);
            ImGui::Checkbox("draw floor", &m_DrawFloor);
            ImGui::Checkbox("draw rims", &m_DrawRims);
            ImGui::InputFloat3("light color", glm::value_ptr(m_LightColor));
            ImGui::InputFloat4("background color", glm::value_ptr(m_SceneBgColor));
            ImGui::InputFloat("rim thickness", &m_RimThickness);
            ImGui::ColorEdit4("rim rgba", glm::value_ptr(m_RimRgba));
            ImGui::End();

            m_LogPanel.draw();
            m_PerfPanel.draw();
        }
    }

private:
    UID m_ID;
    TabHost* m_Parent;

    std::vector<NewDecoration> m_Decorations = GenerateDecorations();
    PolarPerspectiveCamera m_PolarCamera = CreateCameraWithRadius(5.0f);
    bool m_DrawNormals = false;
    bool m_DrawFloor = true;
    bool m_DrawRims = true;
    glm::vec3 m_LightColor = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    glm::vec4 m_SceneBgColor = {0.89f, 0.89f, 0.89f, 1.0f};
    float m_RimThickness = 1.0f;
    glm::vec4 m_RimRgba = {1.0f, 0.4f, 0.0f, 0.85f};

    experimental::Material m_SceneColoredElementsMaterial
    {
        experimental::Shader
        {
            App::slurp("shaders/ExperimentOpenSim.vert"),
            App::slurp("shaders/ExperimentOpenSim.frag"),
        }
    };

    experimental::Material m_SceneTexturedElementsMaterial
    {
        experimental::Shader
        {
            App::slurp("shaders/ExperimentOpenSimTextured.vert"),
            App::slurp("shaders/ExperimentOpenSimTextured.frag"),
        }
    };

    experimental::Material m_SolidColorMaterial
    {
        experimental::Shader
        {
            App::slurp("shaders/ExperimentOpenSimSolidColor.vert"),
            App::slurp("shaders/ExperimentOpenSimSolidColor.frag"),
        }
    };

    experimental::Material m_EdgeDetectorMaterial
    {
        experimental::Shader
        {
            App::slurp("shaders/ExperimentOpenSimEdgeDetect.vert"),
            App::slurp("shaders/ExperimentOpenSimEdgeDetect.frag"),
        }
    };

    experimental::Material m_NormalsMaterial
    {
        experimental::Shader
        {
            App::slurp("shaders/ExperimentGeometryShaderNormals.vert"),
            App::slurp("shaders/ExperimentGeometryShaderNormals.geom"),
            App::slurp("shaders/ExperimentGeometryShaderNormals.frag"),
        }
    };

    experimental::Mesh m_QuadMesh = osc::experimental::LoadMeshFromMeshData(osc::GenTexturedQuad());
    experimental::Texture2D m_FloorTexture = GenChequeredFloorTexture();
    osc::Transform m_FloorTransform = GetFloorTransform();
    std::optional<experimental::RenderTexture> m_SceneTex;
    std::optional<experimental::RenderTexture> m_SelectedTex;
    experimental::Camera m_Camera;

    LogViewerPanel m_LogPanel{"log"};
    PerfPanel m_PerfPanel{"perf"};
};


// public API (PIMPL)

osc::RendererOpenSimTab::RendererOpenSimTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::RendererOpenSimTab::RendererOpenSimTab(RendererOpenSimTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererOpenSimTab& osc::RendererOpenSimTab::operator=(RendererOpenSimTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::RendererOpenSimTab::~RendererOpenSimTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::RendererOpenSimTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererOpenSimTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererOpenSimTab::implParent() const
{
    return m_Impl->getParent();
}

void osc::RendererOpenSimTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererOpenSimTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererOpenSimTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererOpenSimTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererOpenSimTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererOpenSimTab::implOnDraw()
{
    m_Impl->onDraw();
}
