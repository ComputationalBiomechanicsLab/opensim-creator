#include "LOGLCoordinateSystemsTab.hpp"

#include <SDL_events.h>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/Eulers.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/UnitVec3.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>
#include <oscar_learnopengl/MouseCapturingCamera.hpp>

#include <array>
#include <memory>

using namespace osc::literals;
using osc::App;
using osc::Camera;
using osc::ColorSpace;
using osc::CStringView;
using osc::ImageLoadingFlags;
using osc::LoadTexture2DFromImage;
using osc::Material;
using osc::MouseCapturingCamera;
using osc::Shader;
using osc::UnitVec3;
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

    MouseCapturingCamera CreateCameraThatMatchesLearnOpenGL()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.2f, 0.3f, 0.3f, 1.0f});
        return rv;
    }

    Material MakeBoxMaterial()
    {
        Material rv{Shader{
            App::slurp("oscar_learnopengl/shaders/GettingStarted/CoordinateSystems.vert"),
            App::slurp("oscar_learnopengl/shaders/GettingStarted/CoordinateSystems.frag"),
        }};

        rv.setTexture(
            "uTexture1",
            LoadTexture2DFromImage(
                App::resource("oscar_learnopengl/textures/container.jpg"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            )
        );

        rv.setTexture(
            "uTexture2",
            LoadTexture2DFromImage(
                App::resource("oscar_learnopengl/textures/awesomeface.png"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            )
        );

        return rv;
    }
}

class osc::LOGLCoordinateSystemsTab::Impl final : public StandardTabImpl {
public:

    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_Camera.onMount();
    }

    void implOnUnmount() final
    {
        m_Camera.onUnmount();
        App::upd().makeMainEventLoopWaiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_Camera.onEvent(e);
    }

    void implOnTick() final
    {
        double const dt = App::get().getFrameDeltaSinceAppStartup().count();
        m_Step1Transform.rotation = AngleAxis(50_deg * dt, UnitVec3{0.5f, 1.0f, 0.0f});
    }

    void implOnDraw() final
    {
        m_Camera.onDraw();
        draw3DScene();
        draw2DUI();
    }

    void draw3DScene()
    {
        // clear screen and ensure camera has correct pixel rect
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());

        // draw 3D scene
        if (m_ShowStep1) {
            Graphics::DrawMesh(m_Mesh, m_Step1Transform, m_Material, m_Camera);
        }
        else {
            UnitVec3 const axis{1.0f, 0.3f, 0.5f};

            for (size_t i = 0; i < c_CubePositions.size(); ++i) {
                Graphics::DrawMesh(
                    m_Mesh,
                    Transform{
                        .rotation = AngleAxis(i * 20_deg, axis),
                        .position = c_CubePositions[i],
                    },
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
        if (m_Camera.isCapturingMouse()) {
            ImGui::Text("mouse captured (esc to uncapture)");
        }

        Vec3 const cameraPos = m_Camera.getPosition();
        ImGui::Text("camera pos = (%f, %f, %f)", cameraPos.x, cameraPos.y, cameraPos.z);
        Vec<3, Degrees> const cameraEulers = m_Camera.eulers();
        ImGui::Text("camera eulers = (%f, %f, %f)", cameraEulers.x.count(), cameraEulers.y.count(), cameraEulers.z.count());
        ImGui::End();

        m_PerfPanel.onDraw();
    }

    Material m_Material = MakeBoxMaterial();
    Mesh m_Mesh = GenerateLearnOpenGLCubeMesh();
    MouseCapturingCamera m_Camera = CreateCameraThatMatchesLearnOpenGL();

    bool m_ShowStep1 = false;
    Transform m_Step1Transform;

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
