#include "RendererNormalMappingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/MeshTopology.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Cpp20Shims.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <nonstd/span.hpp>
#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace
{
    // matches the quad used in LearnOpenGL's normal mapping tutorial
    osc::Mesh GenerateQuad()
    {
        std::vector<glm::vec3> const verts =
        {
            {-1.0f,  1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            { 1.0f, -1.0f, 0.0f},
            { 1.0f,  1.0f, 0.0f},
        };

        std::vector<glm::vec3> const normals =
        {
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
        };

        std::vector<glm::vec2> const texCoords =
        {
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
        };

        std::vector<uint16_t> const indices =
        {
            0, 1, 2,
            0, 2, 3,
        };

        std::vector<glm::vec4> const tangents = osc::CalcTangentVectors(
            osc::MeshTopology::Triangles,
            verts,
            normals,
            texCoords,
            osc::MeshIndicesView{indices.data(), indices.size()}
        );
        OSC_ASSERT_ALWAYS(tangents.size() == verts.size());

        osc::Mesh rv;
        rv.setVerts(verts);
        rv.setNormals(normals);
        rv.setTexCoords(texCoords);
        rv.setTangents(tangents);
        rv.setIndices(indices);
        return rv;
    }
}

class osc::RendererNormalMappingTab::Impl final {
public:

    Impl()
    {
        m_NormalMappingMaterial.setTexture("uDiffuseMap", m_DiffuseMap);
        m_NormalMappingMaterial.setTexture("uNormalMap", m_NormalMap);

        // these roughly match what LearnOpenGL default to
        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);

        m_LightTransform.position = {0.5f, 1.0f, 0.3f};
        m_LightTransform.scale *= 0.2f;
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_COOKIE " NormalMapping (LearnOpenGL)";
    }

    void onMount()
    {
        m_IsMouseCaptured = true;
    }

    void onUnmount()
    {
        m_IsMouseCaptured = false;
        App::upd().setShowCursor(true);
    }

    bool onEvent(SDL_Event const& e)
    {
        // handle mouse capturing
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            m_IsMouseCaptured = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && osc::IsMouseInMainViewportWorkspaceScreenRect())
        {
            m_IsMouseCaptured = true;
            return true;
        }
        return false;
    }

    void onTick()
    {
        // rotate the quad over time
        AppClock::duration const dt = App::get().getDeltaSinceAppStartup();
        float const angle = glm::radians(-10.0f * dt.count());
        glm::vec3 const axis = glm::normalize(glm::vec3{1.0f, 0.0f, 1.0f});
        m_QuadTransform.rotation = glm::normalize(glm::quat{angle, axis});
    }

    void onDraw()
    {
        // handle mouse capturing and update camera
        if (m_IsMouseCaptured)
        {
            UpdateEulerCameraFromImGuiUserInput(m_Camera, m_CameraEulers);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }

        // clear screen and ensure camera has correct pixel rect
        App::upd().clearScreen({0.1f, 0.1f, 0.1f, 1.0f});

        // draw normal-mapped quad
        {
            m_NormalMappingMaterial.setVec3("uLightWorldPos", m_LightTransform.position);
            m_NormalMappingMaterial.setVec3("uViewWorldPos", m_Camera.getPosition());
            m_NormalMappingMaterial.setBool("uEnableNormalMapping", m_IsNormalMappingEnabled);
            Graphics::DrawMesh(m_QuadMesh, m_QuadTransform, m_NormalMappingMaterial, m_Camera);
        }

        // draw light source
        {
            m_LightCubeMaterial.setVec3("uLightColor", {1.0f, 1.0f, 1.0f});
            Graphics::DrawMesh(m_CubeMesh, m_LightTransform, m_LightCubeMaterial, m_Camera);
        }

        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();

        ImGui::Begin("controls");
        ImGui::Checkbox("normal mapping", &m_IsNormalMappingEnabled);
        ImGui::End();
    }

private:
    UID m_TabID;
    bool m_IsMouseCaptured = false;

    // rendering state
    Material m_NormalMappingMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentNormalMapping.vert"),
            App::slurp("shaders/ExperimentNormalMapping.frag"),
        },
    };
    Material m_LightCubeMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentLightCube.vert"),
            App::slurp("shaders/ExperimentLightCube.frag"),
        },
    };
    Mesh m_CubeMesh = GenLearnOpenGLCube();
    Mesh m_QuadMesh = GenerateQuad();
    Texture2D m_DiffuseMap = LoadTexture2DFromImage(App::resource("textures/brickwall.jpg"));
    Texture2D m_NormalMap = LoadTexture2DFromImage(App::resource("textures/brickwall_normal.jpg"));

    // scene state
    Camera m_Camera;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    Transform m_QuadTransform;
    Transform m_LightTransform;
    bool m_IsNormalMappingEnabled = true;
};


// public API (PIMPL)

osc::CStringView osc::RendererNormalMappingTab::id() noexcept
{
    return "Renderer/NormalMapping";
}

osc::RendererNormalMappingTab::RendererNormalMappingTab(std::weak_ptr<TabHost>) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::RendererNormalMappingTab::RendererNormalMappingTab(RendererNormalMappingTab&&) noexcept = default;
osc::RendererNormalMappingTab& osc::RendererNormalMappingTab::operator=(RendererNormalMappingTab&&) noexcept = default;
osc::RendererNormalMappingTab::~RendererNormalMappingTab() noexcept = default;

osc::UID osc::RendererNormalMappingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererNormalMappingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererNormalMappingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererNormalMappingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererNormalMappingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererNormalMappingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererNormalMappingTab::implOnDraw()
{
    m_Impl->onDraw();
}
