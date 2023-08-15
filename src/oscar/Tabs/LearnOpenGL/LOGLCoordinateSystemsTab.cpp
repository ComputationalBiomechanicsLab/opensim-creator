#include "LOGLCoordinateSystemsTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/Constants.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Panels/PerfPanel.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Utils/Cpp20Shims.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <glm/vec3.hpp>
#include <SDL_events.h>

#include <cstdint>
#include <memory>

namespace
{
    // worldspace positions of each cube (step 2)
    constexpr auto c_CubePositions = osc::to_array<glm::vec3>(
    {
        { 0.0f,  0.0f,  0.0f },
        { 2.0f,  5.0f, -15.0f},
        {-1.5f, -2.2f, -2.5f },
        {-3.8f, -2.0f, -12.3f},
        { 2.4f, -0.4f, -3.5f },
        {-1.7f,  3.0f, -7.5f },
        { 1.3f, -2.0f, -2.5f },
        { 1.5f,  2.0f, -2.5f },
        { 1.5f,  0.2f, -1.5f },
        {-1.3f,  1.0f, -1.5  },
    });

    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/CoordinateSystems";
}

class osc::LOGLCoordinateSystemsTab::Impl final {
public:

    Impl()
    {
        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_Camera.setBackgroundColor({0.2f, 0.3f, 0.3f, 1.0f});
        m_Material.setTexture(
            "uTexture1",
            LoadTexture2DFromImage(
                App::resource("textures/container.jpg"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            )
        );
        m_Material.setTexture(
            "uTexture2",
            LoadTexture2DFromImage(
                App::resource("textures/awesomeface.png"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            )
        );
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
    }

    void onMount()
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void onUnmount()
    {
        m_IsMouseCaptured = false;
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
    }

    bool onEvent(SDL_Event const& e)
    {
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
        float const rotationSpeed = glm::radians(50.0f);
        double const dt = App::get().getFrameDeltaSinceAppStartup().count();
        float const angle = static_cast<float>(rotationSpeed * dt);
        glm::vec3 const axis = glm::normalize(glm::vec3{0.5f, 1.0f, 0.0f});

        m_Step1.rotation = glm::angleAxis(angle, axis);
    }

    void onDraw()
    {
        // handle mouse capturing
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
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());

        // draw 3D scene
        if (m_ShowStep1)
        {
            Graphics::DrawMesh(m_Mesh, m_Step1, m_Material, m_Camera);
        }
        else
        {
            int32_t i = 0;
            glm::vec3 const axis = glm::normalize(glm::vec3{1.0f, 0.3f, 0.5f});
            for (glm::vec3 const& pos : c_CubePositions)
            {
                float const angle = glm::radians(i++ * 20.0f);

                Transform t;
                t.rotation = glm::angleAxis(angle, axis);
                t.position = pos;

                Graphics::DrawMesh(m_Mesh, t, m_Material, m_Camera);
            }
        }

        m_Camera.renderToScreen();

        // draw UI extras
        {
            ImGui::Begin("Tutorial Step");
            ImGui::Checkbox("step1", &m_ShowStep1);
            if (m_IsMouseCaptured)
            {
                ImGui::Text("mouse captured (esc to uncapture)");
            }

            glm::vec3 cameraPos = m_Camera.getPosition();
            ImGui::Text("camera pos = (%f, %f, %f)", cameraPos.x, cameraPos.y, cameraPos.z);
            glm::vec3 cameraEulers = glm::degrees(m_CameraEulers);
            ImGui::Text("camera eulers = (%f, %f, %f)", cameraEulers.x, cameraEulers.y, cameraEulers.z);
            ImGui::End();

            m_PerfPanel.onDraw();
        }
    }

private:
    UID m_TabID;

    Material m_Material
    {
        Shader
        {
            App::slurp("shaders/ExperimentCoordinateSystems.vert"),
            App::slurp("shaders/ExperimentCoordinateSystems.frag"),
        },
    };
    Mesh m_Mesh = GenLearnOpenGLCube();
    Camera m_Camera;
    bool m_IsMouseCaptured = false;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};

    bool m_ShowStep1 = false;
    Transform m_Step1;

    PerfPanel m_PerfPanel{"perf"};
};


// public API (PIMPL)

osc::CStringView osc::LOGLCoordinateSystemsTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLCoordinateSystemsTab::LOGLCoordinateSystemsTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLCoordinateSystemsTab::LOGLCoordinateSystemsTab(LOGLCoordinateSystemsTab&&) noexcept = default;
osc::LOGLCoordinateSystemsTab& osc::LOGLCoordinateSystemsTab::operator=(LOGLCoordinateSystemsTab&&) noexcept = default;
osc::LOGLCoordinateSystemsTab::~LOGLCoordinateSystemsTab() noexcept = default;

osc::UID osc::LOGLCoordinateSystemsTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLCoordinateSystemsTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLCoordinateSystemsTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLCoordinateSystemsTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLCoordinateSystemsTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLCoordinateSystemsTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLCoordinateSystemsTab::implOnDraw()
{
    m_Impl->onDraw();
}
