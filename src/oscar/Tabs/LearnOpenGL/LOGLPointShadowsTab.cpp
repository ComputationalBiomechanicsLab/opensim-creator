#include "LOGLPointShadowsTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/RenderTexture.hpp"
#include "oscar/Graphics/RenderTextureDescriptor.hpp"
#include "oscar/Graphics/RenderTextureReadWrite.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Utils/Algorithms.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <chrono>
#include <cmath>
#include <string>
#include <utility>

namespace
{
    constexpr glm::ivec2 c_ShadowmapDims = {1024, 1024};
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/PointShadows";

    constexpr osc::Transform MakeTransform(float scale, glm::vec3 position)
    {
        osc::Transform rv;
        rv.scale = glm::vec3(scale);
        rv.position = position;
        return rv;
    }

    osc::Transform MakeRotatedTransform()
    {
        osc::Transform rv;
        rv.scale = glm::vec3(0.75f);
        rv.rotation = glm::angleAxis(glm::radians(60.0f), glm::normalize(glm::vec3{1.0f, 0.0f, 1.0f}));
        rv.position = {-1.5f, 2.0f, -3.0f};
        return rv;
    }

    auto const c_CubeTransforms = osc::MakeArray<osc::Transform>(
        MakeTransform(0.5f, {4.0f, -3.5f, 0.0f}),
        MakeTransform(0.75f, {2.0f, 3.0f, 1.0f}),
        MakeTransform(0.5f, {-3.0f, -1.0f, 0.0f}),
        MakeTransform(0.5f, {-1.5f, 1.0f, 1.5f}),
        MakeRotatedTransform()
    );

    osc::Camera CreateSceneCamera()
    {
        osc::Camera rv;
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        return rv;
    }

    osc::Camera CreateShadowmappingCamera()
    {
        osc::Camera rv;
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        rv.setCameraFOV(glm::radians(90.0f));
        rv.setNearClippingPlane(1.0f);
        rv.setFarClippingPlane(25.0f);
        return rv;
    }

    osc::RenderTexture CreateDepthTexture()
    {
        // TODO: this needs to be a cubemap
        osc::RenderTextureDescriptor desc{c_ShadowmapDims};
        desc.setReadWrite(osc::RenderTextureReadWrite::Linear);
        return osc::RenderTexture{std::move(desc)};
    }
}

class osc::LOGLPointShadowsTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
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
        App::upd().makeMainEventLoopWaiting();
        App::upd().setShowCursor(true);
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
        // move light position over time
        float const seconds = App::get().getDeltaSinceAppStartup().count();
        m_LightPos.z = 3.0f * std::sin(0.5f * seconds);
    }

    void onDrawMainMenu() {}

    void onDraw()
    {
        handleMouseCapture();
        draw3DScene();
    }

private:
    void handleMouseCapture()
    {
        if (m_IsMouseCaptured)
        {
            UpdateEulerCameraFromImGuiUserInput(m_SceneCamera, m_CameraEulers);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }
    }

    void draw3DScene()
    {
        drawShadowPassToCubemap();
        drawShadowmappedSceneToScreen();
    }

    void drawShadowPassToCubemap()
    {
        // create mat4 shadowTransforms[6];
        // create projection matrix with FoV=90deg, 1 near, 25 far
        // for each cube face:
        //   create shadowTransform:
        //     projection * lookAt(from=lightPos, to=lightPos+axis, up=appropriateUp)
        // assign shadowTransforms to shadow mapping material
        //   the shadow mapping material uses each to create NDC for each cube face
    }

    void drawShadowmappedSceneToScreen()
    {
        // render: render scene to screen as normal, using the depth cubemaps for shadow mapping
    }

    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    Material m_ShadowMappingMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentPointShadows.vert"),
            App::slurp("shaders/ExperimentPointShadows.geom"),
            App::slurp("shaders/ExperimentPointShadows.frag"),
        },
    };

    Camera m_SceneCamera = CreateSceneCamera();
    bool m_IsMouseCaptured = false;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    Texture2D m_WoodTexture = LoadTexture2DFromImage(
        App::resource("textures/wood.png"),
        ColorSpace::sRGB
    );
    Mesh m_CubeMesh = GenCube();
    RenderTexture m_DepthTexture = CreateDepthTexture();
    glm::vec3 m_LightPos = {0.0f, 0.0f, 0.0f};
};


// public API (PIMPL)

osc::CStringView osc::LOGLPointShadowsTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLPointShadowsTab::LOGLPointShadowsTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::LOGLPointShadowsTab::LOGLPointShadowsTab(LOGLPointShadowsTab&&) noexcept = default;
osc::LOGLPointShadowsTab& osc::LOGLPointShadowsTab::operator=(LOGLPointShadowsTab&&) noexcept = default;
osc::LOGLPointShadowsTab::~LOGLPointShadowsTab() noexcept = default;

osc::UID osc::LOGLPointShadowsTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLPointShadowsTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLPointShadowsTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLPointShadowsTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLPointShadowsTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLPointShadowsTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLPointShadowsTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLPointShadowsTab::implOnDraw()
{
    m_Impl->onDraw();
}
