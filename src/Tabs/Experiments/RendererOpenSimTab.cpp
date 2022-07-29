#include "RendererOpenSimTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/Mesh.hpp"
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
            rv.emplace_back(dec);
        }
        return rv;
    }
}

class osc::RendererOpenSimTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{parent}
    {
        m_Camera.setBackgroundColor({0.89f, 0.89f, 0.89f, 1.0f});

        m_Material.setVec3("uLightColor", {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f});

        log::info("%s", osc::StreamToString(m_Material.getShader()).c_str());

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
        Rect screenRect = osc::GetMainViewportWorkspaceScreenRect();
        glm::vec2 screenDims = osc::Dimensions(screenRect);
        m_Camera.setPixelRect(screenRect);

        osc::UpdatePolarCameraFromImGuiUserInput(screenDims, m_PolarCamera);
        m_Camera.setPosition(m_PolarCamera.getPos());
        m_Camera.setViewMatrix(m_PolarCamera.getViewMtx());
        m_Camera.setProjectionMatrix(m_PolarCamera.getProjMtx(AspectRatio(screenDims)));

        m_Material.setVec3("uViewPos", m_PolarCamera.getPos());
        recomputeSceneLightPosition();

        for (NewDecoration const& dec : m_Decorations)
        {
            osc::experimental::MaterialPropertyBlock b;
            b.setVec4("uDiffuseColor", dec.Color);
            osc::experimental::Graphics::DrawMesh(*dec.Mesh, dec.Transform, m_Material, m_Camera, std::move(b));
        }
        m_Camera.render();

        // TODO: render selected geometry (for rim highlighting)
        // TODO: perform rim highlight
        // TODO: allow normals visualization
        // TODO: allow grid visualization

        m_LogPanel.draw();
        m_PerfPanel.draw();
    }

private:
    void recomputeSceneLightPosition()
    {
        // automatically change lighting position based on camera position
        glm::vec3 p = glm::normalize(-m_PolarCamera.focusPoint - m_PolarCamera.getPos());
        glm::vec3 up = {0.0f, 1.0f, 0.0f};
        glm::vec3 mp = glm::rotate(glm::mat4{1.0f}, 1.05f * fpi4, up) * glm::vec4{p, 0.0f};
        m_Material.setVec3("uLightDir", glm::normalize(mp + -up));
    }

    UID m_ID;
    TabHost* m_Parent;
    std::vector<NewDecoration> m_Decorations = GenerateDecorations();
    PolarPerspectiveCamera m_PolarCamera;

    experimental::Shader m_Shader
    {
        App::slurp("shaders/ExperimentOpenSim.vert"),
        App::slurp("shaders/ExperimentOpenSim.frag"),
    };
    experimental::Material m_Material{m_Shader};
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
