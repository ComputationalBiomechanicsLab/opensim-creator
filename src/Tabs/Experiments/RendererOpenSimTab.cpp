#include "RendererOpenSimTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/ComponentDecoration.hpp"
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
    std::unordered_map<std::shared_ptr<osc::Mesh>, std::shared_ptr<osc::experimental::Mesh const>> g_MeshLookup;

    std::shared_ptr<osc::experimental::Mesh const> GetExperimentalMesh(std::shared_ptr<osc::Mesh> const& m)
    {
        auto [it, inserted] = g_MeshLookup.try_emplace(m);
        if (inserted)
        {
            it->second = std::make_shared<osc::experimental::Mesh>(osc::experimental::LoadMeshFromLegacyMesh(*m));
        }
        return it->second;
    }

    struct NewDecoration final {
        std::shared_ptr<osc::experimental::Mesh const> Mesh;
        osc::Transform Transform;
        glm::vec4 Color;
        bool IsHovered;

        NewDecoration(osc::ComponentDecoration const& d) :
            Mesh{GetExperimentalMesh(d.mesh)},
            Transform{d.transform},
            Color{d.color},
            IsHovered{static_cast<bool>(d.flags & osc::ComponentDecorationFlags_IsHovered)}
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
        std::vector<osc::ComponentDecoration> decs;
        osc::GenerateModelDecorations(p, decs);
        std::vector<NewDecoration> rv;
        rv.reserve(decs.size());
        for (osc::ComponentDecoration const& dec : decs)
        {
            auto& d = rv.emplace_back(dec);
            if (dec.component && dec.component->getName() == "torso_geom_4")
            {
                d.IsHovered = true;
            }
        }
        return rv;
    }

    osc::experimental::Texture2D GenChequeredFloorTexture()
    {
        constexpr size_t chequer_width = 32;
        constexpr size_t chequer_height = 32;
        constexpr size_t w = 2 * chequer_width;
        constexpr size_t h = 2 * chequer_height;

        constexpr osc::Rgba32 on_color = {0xff, 0xff, 0xff, 0xff};
        constexpr osc::Rgba32 off_color = {0xf3, 0xf3, 0xf3, 0xff};

        std::unique_ptr<osc::Rgba32[]> pixels{new osc::Rgba32[w * h]};

        for (size_t row = 0; row < h; ++row)
        {
            size_t row_start = row * w;
            bool y_on = (row / chequer_height) % 2 == 0;
            for (size_t col = 0; col < w; ++col)
            {
                bool x_on = (col / chequer_width) % 2 == 0;
                pixels[row_start + col] = y_on ^ x_on ? on_color : off_color;
            }
        }

        return osc::experimental::Texture2D(static_cast<int>(w), static_cast<int>(h), nonstd::span<uint8_t const>{&pixels[0].r, 4*w*h}, 4);
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
        // compute viewport stuff (the render fills the viewport)
        Rect const viewportRect = osc::GetMainViewportWorkspaceScreenRect();
        glm::vec2 const viewportRectDims = osc::Dimensions(viewportRect);

        // configure render textures to match viewport dimensions
        {
            experimental::RenderTextureDescriptor desc = osc::experimental::RenderTextureDescriptor
            {
                static_cast<int>(viewportRectDims.x),
                static_cast<int>(viewportRectDims.y),
            };

            desc.setAntialiasingLevel(osc::App::get().getMSXAASamplesRecommended());
            EmplaceOrReformat(m_SelectedTex, desc);
            EmplaceOrReformat(m_SceneTex, desc);
        }

        // update the (purely mathematical) polar camera from user input
        osc::UpdatePolarCameraFromImGuiUserInput(viewportRectDims, m_PolarCamera);
        glm::vec3 const lightDir = RecommendedLightDirection(m_PolarCamera);

        // update the (rendered to) scene camera
        m_Camera.setPosition(m_PolarCamera.getPos());
        m_Camera.setViewMatrix(m_PolarCamera.getViewMtx());
        m_Camera.setProjectionMatrix(m_PolarCamera.getProjMtx(AspectRatio(viewportRectDims)));

        // render selected objects as a solid color to a seperate texture
        AABB rimAABBWorldspace = InvertedAABB();
        bool hasRims = false;
        if (m_DrawRims)
        {
            for (NewDecoration const& dec : m_Decorations)
            {
                if (dec.IsHovered)
                {
                    experimental::Graphics::DrawMesh(*dec.Mesh, dec.Transform, m_SolidColorMaterial, m_Camera);

                    rimAABBWorldspace = Union(rimAABBWorldspace, WorldpaceAABB(dec));
                    hasRims = true;
                }
            }

            // TODO: resize texture based on AABB
            // Rect rimRectNDC = WorldpsaceAABBToNdcRect(rimAABBWorldspace, m_Camera.getViewProjectionMatrix());
            // rimRectNDC = Expand(rimRectNDC, rimThicknessNDC);
            // Rect rimScreenRect = NdcRectToScreenspaceViewportRect(rimRectNDC, Rect{{}, viewportRectDims});

            m_Camera.setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});
            m_Camera.swapTexture(m_SelectedTex);
            m_Camera.render();
            m_Camera.swapTexture(m_SelectedTex);
            m_Camera.setBackgroundColor(m_SceneBgColor);
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

                experimental::Graphics::DrawMesh(m_QuadMesh, m_FloorTransform, m_SceneTexturedElementsMaterial, m_Camera);
            }

            // if rims are requested, draw them
            if (hasRims)
            {
                glm::vec2 rimThicknessNDC = glm::vec2{m_RimThickness} / glm::vec2{viewportRectDims};

                m_EdgeDetectorMaterial.setRenderTexture("uScreenTexture", *m_SelectedTex);
                m_EdgeDetectorMaterial.setVec4("uRimRgba", m_RimRgba);
                m_EdgeDetectorMaterial.setVec2("uRimThickness", rimThicknessNDC);
                m_EdgeDetectorMaterial.setTransparent(true);
                m_EdgeDetectorMaterial.setDepthTested(false);

                experimental::Graphics::DrawMesh(m_QuadMesh, m_Camera.getInverseViewProjectionMatrix(), m_EdgeDetectorMaterial, m_Camera);

                m_EdgeDetectorMaterial.clearRenderTexture("uScreenTexture");  // prevents copies on next frame
            }

            m_Camera.swapTexture(m_SceneTex);
            m_Camera.render();
            m_Camera.swapTexture(m_SceneTex);
        }

        // blit the anti-aliased render to the screen
        BlitToScreen(*m_SceneTex, viewportRect);

        // render auxiliary 2D UI
        {
            ImGui::Begin("controls");
            ImGui::Checkbox("draw normals", &m_DrawNormals);
            ImGui::Checkbox("draw floor", &m_DrawFloor);
            ImGui::Checkbox("draw rims", &m_DrawRims);
            ImGui::InputFloat3("light color", glm::value_ptr(m_LightColor));
            ImGui::InputFloat4("background color", glm::value_ptr(m_SceneBgColor));
            ImGui::InputFloat("rim thickness", &m_RimThickness);
            ImGui::InputFloat4("rim rgba", glm::value_ptr(m_RimRgba));
            ImGui::End();

            m_LogPanel.draw();
            m_PerfPanel.draw();
        }
    }

private:
    void BlitToScreen(experimental::RenderTexture const& renderTexture, Rect const& screenRect)
    {
        experimental::Camera c;
        c.setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});
        c.setPixelRect(screenRect);
        c.setProjectionMatrix(glm::mat4{1.0f});
        c.setViewMatrix(glm::mat4{1.0f});
        c.setClearFlags(experimental::CameraClearFlags::Depth);

        m_QuadMaterial.setRenderTexture("uTexture", renderTexture);
        m_QuadMaterial.setTransparent(true);
        experimental::Graphics::DrawMesh(m_QuadMesh, Transform{}, m_QuadMaterial, c);
        c.render();
        m_QuadMaterial.clearRenderTexture("uTexture");
    }

    UID m_ID;
    TabHost* m_Parent;

    std::vector<NewDecoration> m_Decorations = GenerateDecorations();
    PolarPerspectiveCamera m_PolarCamera = CreateCameraWithRadius(5.0f);
    bool m_DrawNormals = false;
    bool m_DrawFloor = true;
    bool m_DrawRims = true;
    glm::vec3 m_LightColor = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    glm::vec4 m_SceneBgColor = {0.89f, 0.89f, 0.89f, 1.0f};
    float m_RimThickness = 1.5f;
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
            App::slurp("shaders/ExperimentOpensimSolidColor.vert"),
            App::slurp("shaders/ExperimentOpensimSolidColor.frag"),
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

    experimental::Material m_QuadMaterial
    {
        experimental::Shader
        {
            App::slurp("shaders/ExperimentOpenSimQuadSampler.vert"),
            App::slurp("shaders/ExperimentOpenSimQuadSampler.frag"),
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
