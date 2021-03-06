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

    void EmplaceOrReformat(std::optional<osc::experimental::RenderTexture>& t, osc::experimental::RenderTextureDescriptor const& desc)
    {
        if (t)
        {
            t->reformat(desc);
        }
        else
        {
            t.emplace(desc);
        }
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
        // configure render textures owned by this tab
        osc::Rect viewportRect = osc::GetMainViewportWorkspaceScreenRect();
        glm::vec2 viewportRectDims = osc::Dimensions(viewportRect);
        osc::experimental::RenderTextureDescriptor desc = osc::experimental::RenderTextureDescriptor
        {
            static_cast<int>(viewportRectDims.x),
            static_cast<int>(viewportRectDims.y),
        };
        desc.setAntialiasingLevel(osc::App::get().getMSXAASamplesRecommended());
        EmplaceOrReformat(m_SceneTex, desc);
        EmplaceOrReformat(m_SelectedTex, desc);
        EmplaceOrReformat(m_RimsTex, desc);

        // update polar camera from user input
        osc::UpdatePolarCameraFromImGuiUserInput(viewportRectDims, m_PolarCamera);

        // update the rendered-to scene camera
        m_Camera.setPosition(m_PolarCamera.getPos());
        m_Camera.setViewMatrix(m_PolarCamera.getViewMtx());
        m_Camera.setProjectionMatrix(m_PolarCamera.getProjMtx(AspectRatio(viewportRectDims)));
        m_Camera.setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});

        // render OpenSim scene to tab-owned texture
        m_Material.setVec3("uViewPos", m_PolarCamera.getPos());
        m_Material.setVec3("uLightDir", lightPosition());
        m_Material.setVec3("uLightColor", {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f});
        for (NewDecoration const& dec : m_Decorations)
        {
            osc::experimental::MaterialPropertyBlock b;
            b.setVec4("uDiffuseColor", dec.Color);
            osc::experimental::Graphics::DrawMesh(*dec.Mesh, dec.Transform, m_Material, m_Camera, std::move(b));

            // (if applicable): also append normals shader element
        }
        // (if applicable): draw the textured floor
        m_Camera.swapTexture(m_SceneTex);
        m_Camera.render();
        m_Camera.swapTexture(m_SceneTex);
        OSC_ASSERT(m_SceneTex.has_value());

        // render selected objects as a solid color to a seperate texture
        m_SolidColorMaterial.setVec3("uViewPos", m_PolarCamera.getPos());
        m_SolidColorMaterial.setVec3("uLightColor", {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f});
        m_SolidColorMaterial.setVec3("uLightDir", lightPosition());
        osc::experimental::MaterialPropertyBlock b;
        b.setVec4("uDiffuseColor", {1.0f, 0.0f, 0.0f, 1.0f});
        for (NewDecoration const& dec : m_Decorations)
        {
            if (dec.IsHovered)
            {
                osc::experimental::Graphics::DrawMesh(*dec.Mesh, dec.Transform, m_SolidColorMaterial, m_Camera, b);
            }
        }
        m_Camera.swapTexture(m_SelectedTex);
        m_Camera.render();
        m_Camera.swapTexture(m_SelectedTex);
        OSC_ASSERT(m_SelectedTex.has_value());

        // sample the solid color texture through an edge-detection kernel to the scene texture
        m_EdgeDetectorMaterial.setRenderTexture("uScreenTexture", *m_SelectedTex);
        m_EdgeDetectorMaterial.setVec4("uRimRgba", {1.0f, 0.4f, 0.0f, 0.85f});
        m_EdgeDetectorMaterial.setFloat("uRimThickness", (glm::vec2{1.5f} / glm::vec2{viewportRectDims}).x);  // TODO: must be vec2
        experimental::Graphics::DrawMesh(m_QuadMesh, Transform{}, m_EdgeDetectorMaterial, m_Camera);

        // TODO: scissor
        m_Camera.setViewMatrix(glm::mat4{1.0f});
        m_Camera.setProjectionMatrix(glm::mat4{1.0f});
        m_Camera.swapTexture(m_RimsTex);
        m_Camera.render();
        m_Camera.swapTexture(m_RimsTex);
        OSC_ASSERT(m_SceneTex.has_value());
        m_EdgeDetectorMaterial.clearRenderTexture("uScreenTexture");
        m_EdgeDetectorMaterial.setTransparent(true);

        // blit scene texture to screen
        m_Camera.setTexture();
        m_Camera.setPixelRect(viewportRect);
        m_Camera.setViewMatrix(glm::mat4{1.0f});
        m_Camera.setProjectionMatrix(glm::mat4{1.0f});
        m_QuadMaterial.setRenderTexture("uScreenTexture", *m_SceneTex);
        osc::experimental::Graphics::DrawMesh(m_QuadMesh, Transform{}, m_QuadMaterial, m_Camera);
        m_Camera.render();
        m_Camera.setPixelRect();
        m_QuadMaterial.clearRenderTexture("uScreenTexture");

        // TODO: allow normals visualization
        // TODO: allow grid visualization

        m_LogPanel.draw();
        m_PerfPanel.draw();
    }

private:
    glm::vec3 lightPosition() const
    {
        // automatically change lighting position based on camera position
        glm::vec3 p = glm::normalize(-m_PolarCamera.focusPoint - m_PolarCamera.getPos());
        glm::vec3 up = {0.0f, 1.0f, 0.0f};
        glm::vec3 mp = glm::rotate(glm::mat4{1.0f}, 1.05f * fpi4, up) * glm::vec4{p, 0.0f};
        return glm::normalize(mp + -up);
    }

    UID m_ID;
    TabHost* m_Parent;

    std::vector<NewDecoration> m_Decorations = GenerateDecorations();
    PolarPerspectiveCamera m_PolarCamera;

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
            App::slurp("shaders/ExperimentFrameBuffersScreen.vert"),
            App::slurp("shaders/ExperimentFrameBuffersScreen.frag"),
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
