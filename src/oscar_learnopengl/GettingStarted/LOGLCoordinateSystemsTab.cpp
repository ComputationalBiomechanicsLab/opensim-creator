#include "LOGLCoordinateSystemsTab.hpp"

#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <memory>
#include <numbers>

using osc::Vec3;

namespace
{
    // worldspace positions of each cube (step 2)
    constexpr auto c_CubePositions = std::to_array<Vec3>(
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
        {-1.3f,  1.0f, -1.5f },
    });

    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/CoordinateSystems";

    osc::Camera CreateCameraThatMatchesLearnOpenGL()
    {
        osc::Camera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(osc::Deg2Rad(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.2f, 0.3f, 0.3f, 1.0f});
        return rv;
    }
}

class osc::LOGLCoordinateSystemsTab::Impl final : public osc::StandardTabBase {
public:

    Impl() : StandardTabBase{c_TabStringID}
    {
        m_Material.setTexture(
            "uTexture1",
            LoadTexture2DFromImage(
                App::resource("oscar_learnopengl/textures/container.jpg"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            )
        );
        m_Material.setTexture(
            "uTexture2",
            LoadTexture2DFromImage(
                App::resource("oscar_learnopengl/textures/awesomeface.png"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            )
        );
    }

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void implOnUnmount() final
    {
        m_IsMouseCaptured = false;
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
    }

    bool implOnEvent(SDL_Event const& e) final
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

    void implOnTick() final
    {
        float const rotationSpeed = Deg2Rad(50.0f);
        double const dt = App::get().getFrameDeltaSinceAppStartup().count();
        auto const angle = static_cast<float>(rotationSpeed * dt);
        Vec3 const axis = Normalize(Vec3{0.5f, 1.0f, 0.0f});

        m_Step1.rotation = AngleAxis(angle, axis);
    }

    void implOnDraw() final
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
            Vec3 const axis = Normalize(Vec3{1.0f, 0.3f, 0.5f});
            for (size_t i = 0; i < c_CubePositions.size(); ++i)
            {
                Vec3 const& pos = c_CubePositions[i];
                float const angle = Deg2Rad(static_cast<float>(i++) * 20.0f);

                Transform t;
                t.rotation = AngleAxis(angle, axis);
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

            Vec3 cameraPos = m_Camera.getPosition();
            ImGui::Text("camera pos = (%f, %f, %f)", cameraPos.x, cameraPos.y, cameraPos.z);
            Vec3 cameraEulers = osc::Rad2Deg(m_CameraEulers);
            ImGui::Text("camera eulers = (%f, %f, %f)", cameraEulers.x, cameraEulers.y, cameraEulers.z);
            ImGui::End();

            m_PerfPanel.onDraw();
        }
    }

    Material m_Material
    {
        Shader
        {
            App::slurp("oscar_learnopengl/shaders/GettingStarted/CoordinateSystems.vert"),
            App::slurp("oscar_learnopengl/shaders/GettingStarted/CoordinateSystems.frag"),
        },
    };
    Mesh m_Mesh = GenLearnOpenGLCube();
    Camera m_Camera = CreateCameraThatMatchesLearnOpenGL();
    bool m_IsMouseCaptured = false;
    Vec3 m_CameraEulers = {};

    bool m_ShowStep1 = false;
    Transform m_Step1;

    PerfPanel m_PerfPanel{"perf"};
};


// public API

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
