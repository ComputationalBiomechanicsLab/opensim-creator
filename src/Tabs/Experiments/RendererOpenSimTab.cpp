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
}

class osc::RendererOpenSimTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{parent}
    {
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

            desc.setAntialiasingLevel(App::get().getMSXAASamplesRecommended());
            EmplaceOrReformat(m_SceneTex, desc);
            EmplaceOrReformat(m_SelectedTex, desc);
            EmplaceOrReformat(m_RimsTex, desc);
        }

        // update the (purely mathematical) polar camera from user input
        osc::UpdatePolarCameraFromImGuiUserInput(viewportRectDims, m_PolarCamera);
        glm::vec3 const lightDir = RecommendedLightDirection(m_PolarCamera);

        // update the (rendered to) scene camera
        m_Camera.setPosition(m_PolarCamera.getPos());
        m_Camera.setViewMatrix(m_PolarCamera.getViewMtx());
        m_Camera.setProjectionMatrix(m_PolarCamera.getProjMtx(AspectRatio(viewportRectDims)));
        m_Camera.setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});

        // render scene to the scene texture
        {
            m_Material.setVec3("uViewPos", m_PolarCamera.getPos());
            m_Material.setVec3("uLightDir", lightDir);
            m_Material.setVec3("uLightColor", {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f});

            experimental::MaterialPropertyBlock propBlock;
            for (NewDecoration const& dec : m_Decorations)
            {
                propBlock.setVec4("uDiffuseColor", dec.Color);
                experimental::Graphics::DrawMesh(*dec.Mesh, dec.Transform, m_Material, m_Camera, propBlock);

                if (m_DrawNormals)
                {
                    experimental::Graphics::DrawMesh(*dec.Mesh, dec.Transform, m_NormalsMaterial, m_Camera);
                }
            }
            // (if applicable): draw the textured floor TODO

            m_Camera.swapTexture(m_SceneTex);
            m_Camera.render();
            m_Camera.swapTexture(m_SceneTex);
        }

        // render selected objects as a solid color to a seperate texture
        {
            m_SolidColorMaterial.setVec3("uViewPos", m_PolarCamera.getPos());
            m_SolidColorMaterial.setVec3("uLightColor", {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f});
            m_SolidColorMaterial.setVec3("uLightDir", lightDir);
            m_SolidColorMaterial.setVec4("uDiffuseColor", {1.0f, 0.0f, 0.0f, 1.0f});

            for (NewDecoration const& dec : m_Decorations)
            {
                if (dec.IsHovered)
                {
                    experimental::Graphics::DrawMesh(*dec.Mesh, dec.Transform, m_SolidColorMaterial, m_Camera);
                }
            }

            m_Camera.swapTexture(m_SelectedTex);
            m_Camera.render();
            m_Camera.swapTexture(m_SelectedTex);
        }

        // sample the solid color through an edge-detection kernel (rim highlighting)
        {
            m_Camera.setViewMatrix(glm::mat4{1.0f});  // it's a fullscreen quad
            m_Camera.setProjectionMatrix(glm::mat4{1.0f});  // it's a fullscreen quad

            m_EdgeDetectorMaterial.setRenderTexture("uScreenTexture", *m_SelectedTex);
            m_EdgeDetectorMaterial.setVec4("uRimRgba", {1.0f, 0.4f, 0.0f, 0.85f});
            m_EdgeDetectorMaterial.setFloat("uRimThickness", (glm::vec2{1.5f} / glm::vec2{viewportRectDims}).x);  // TODO: must be vec2
            m_EdgeDetectorMaterial.setTransparent(true);

            experimental::Graphics::DrawMesh(m_QuadMesh, Transform{}, m_EdgeDetectorMaterial, m_Camera);

            m_Camera.swapTexture(m_RimsTex);
            m_Camera.render();
            m_Camera.swapTexture(m_RimsTex);

            m_EdgeDetectorMaterial.clearRenderTexture("uScreenTexture");  // prevents copies on next frame
        }

        BlitToScreen(*m_SceneTex, viewportRect);
        BlitToScreen(*m_RimsTex, viewportRect);

        // render auxiliary 2D UI
        {
            ImGui::Begin("controls");
            ImGui::Checkbox("draw normals", &m_DrawNormals);
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

    experimental::Material m_Material
    {
        experimental::Shader
        {
            App::slurp("shaders/ExperimentOpenSim.vert"),
            App::slurp("shaders/ExperimentOpenSim.frag"),
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
    std::optional<experimental::RenderTexture> m_SceneTex;
    std::optional<experimental::RenderTexture> m_SelectedTex;
    std::optional<experimental::RenderTexture> m_RimsTex;
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
