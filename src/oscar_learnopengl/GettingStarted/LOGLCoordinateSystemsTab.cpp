#include "LOGLCoordinateSystemsTab.hpp"

#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>

#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/Eulers.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <memory>
#include <numbers>

using namespace osc::literals;
using osc::Camera;
using osc::CStringView;
using osc::Vec3;

namespace
{
    // worldspace positions of each cube (step 2)
    constexpr auto c_CubePositions = std::to_array<Vec3>({
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

    constexpr CStringView c_TabStringID = "LearnOpenGL/CoordinateSystems";

    Camera CreateCameraThatMatchesLearnOpenGL()
    {
        Camera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.2f, 0.3f, 0.3f, 1.0f});
        return rv;
    }
}

class osc::LOGLCoordinateSystemsTab::Impl final : public StandardTabImpl {
public:

    Impl() : StandardTabImpl{c_TabStringID}
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
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            m_IsMouseCaptured = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && IsMouseInMainViewportWorkspaceScreenRect()) {
            m_IsMouseCaptured = true;
            return true;
        }
        return false;
    }

    void implOnTick() final
    {
        double const dt = App::get().getFrameDeltaSinceAppStartup().count();
        auto const angle = 50_deg * dt;
        Vec3 const axis = Normalize(Vec3{0.5f, 1.0f, 0.0f});

        m_Step1.rotation = AngleAxis(angle, axis);
    }

    void implOnDraw() final
    {
        // handle mouse capturing
        if (m_IsMouseCaptured) {
            UpdateEulerCameraFromImGuiUserInput(m_Camera, m_CameraEulers);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }

        draw3DScene();
        draw2DUI();
    }

    void draw3DScene()
    {
        // clear screen and ensure camera has correct pixel rect
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());

        // draw 3D scene
        if (m_ShowStep1) {
            Graphics::DrawMesh(m_Mesh, m_Step1, m_Material, m_Camera);
        }
        else {
            Vec3 const axis = Normalize(Vec3{1.0f, 0.3f, 0.5f});
            for (size_t i = 0; i < c_CubePositions.size(); ++i)
            {
                Vec3 const& pos = c_CubePositions[i];

                Graphics::DrawMesh(
                    m_Mesh,
                    {.rotation = AngleAxis(i++ * 20_deg, axis), .position = pos},
                    m_Material,
                    m_Camera
                );
            }
        }

        m_Camera.renderToScreen();
    }

    void draw2DUI()
    {
        ImGui::Begin("Tutorial Step");
        ImGui::Checkbox("step1", &m_ShowStep1);
        if (m_IsMouseCaptured) {
            ImGui::Text("mouse captured (esc to uncapture)");
        }

        Vec3 const cameraPos = m_Camera.getPosition();
        ImGui::Text("camera pos = (%f, %f, %f)", cameraPos.x, cameraPos.y, cameraPos.z);
        Vec<3, Degrees> const cameraEulers = m_CameraEulers;
        ImGui::Text("camera eulers = (%f, %f, %f)", cameraEulers.x.count(), cameraEulers.y.count(), cameraEulers.z.count());
        ImGui::End();

        m_PerfPanel.onDraw();
    }

    Material m_Material{Shader{
        App::slurp("oscar_learnopengl/shaders/GettingStarted/CoordinateSystems.vert"),
        App::slurp("oscar_learnopengl/shaders/GettingStarted/CoordinateSystems.frag"),
    }};
    Mesh m_Mesh = GenerateLearnOpenGLCubeMesh();
    Camera m_Camera = CreateCameraThatMatchesLearnOpenGL();
    bool m_IsMouseCaptured = false;
    Eulers m_CameraEulers = {};

    bool m_ShowStep1 = false;
    Transform m_Step1;

    PerfPanel m_PerfPanel{"perf"};
};


// public API

CStringView osc::LOGLCoordinateSystemsTab::id()
{
    return c_TabStringID;
}

osc::LOGLCoordinateSystemsTab::LOGLCoordinateSystemsTab(ParentPtr<ITabHost> const&) :
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

CStringView osc::LOGLCoordinateSystemsTab::implGetName() const
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
