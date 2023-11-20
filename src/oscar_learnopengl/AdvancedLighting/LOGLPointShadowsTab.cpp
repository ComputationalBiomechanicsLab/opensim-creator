#include "LOGLPointShadowsTab.hpp"

#include <IconsFontAwesome5.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/RenderTexture.hpp>
#include <oscar/Graphics/RenderTextureDescriptor.hpp>
#include <oscar/Graphics/RenderTextureReadWrite.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Graphics/TextureDimensionality.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <SDL_events.h>

#include <array>
#include <chrono>
#include <cmath>
#include <string>
#include <utility>

using osc::Camera;
using osc::Color;
using osc::CStringView;
using osc::RenderTexture;
using osc::RenderTextureDescriptor;
using osc::RenderTextureFormat;
using osc::RenderTextureReadWrite;
using osc::TextureDimensionality;
using osc::Transform;
using osc::Vec2i;
using osc::Vec3;

namespace
{
    constexpr Vec2i c_ShadowmapDims = {1024, 1024};
    constexpr CStringView c_TabStringID = "LearnOpenGL/PointShadows";

    constexpr Transform MakeTransform(float scale, Vec3 position)
    {
        Transform rv;
        rv.scale = Vec3(scale);
        rv.position = position;
        return rv;
    }

    Transform MakeRotatedTransform()
    {
        Transform rv;
        rv.scale = Vec3(0.75f);
        rv.rotation = osc::AngleAxis(osc::Deg2Rad(60.0f), osc::Normalize(Vec3{1.0f, 0.0f, 1.0f}));
        rv.position = {-1.5f, 2.0f, -3.0f};
        return rv;
    }

    struct SceneCube final {
        explicit SceneCube(Transform transform_) :
            transform{transform_}
        {
        }

        SceneCube(Transform transform_, bool invertNormals_) :
            transform{transform_},
            invertNormals{invertNormals_}
        {
        }

        Transform transform;
        bool invertNormals = false;
    };

    auto MakeSceneCubes()
    {
        return std::to_array<SceneCube>(
        {
            SceneCube{MakeTransform(5.0f, {}), true},
            SceneCube{MakeTransform(0.5f, {4.0f, -3.5f, 0.0f})},
            SceneCube{MakeTransform(0.75f, {2.0f, 3.0f, 1.0f})},
            SceneCube{MakeTransform(0.5f, {-3.0f, -1.0f, 0.0f})},
            SceneCube{MakeTransform(0.5f, {-1.5f, 1.0f, 1.5f})},
            SceneCube{MakeRotatedTransform()},
        });
    }

    RenderTexture CreateDepthTexture()
    {
        RenderTextureDescriptor desc{c_ShadowmapDims};
        desc.setDimensionality(TextureDimensionality::Cube);
        desc.setReadWrite(RenderTextureReadWrite::Linear);
        desc.setColorFormat(RenderTextureFormat::Depth);
        return RenderTexture{desc};
    }

    Camera CreateCamera()
    {
        Camera rv;
        rv.setPosition({0.0f, 0.0f, 5.0f});
        rv.setCameraFOV(osc::Deg2Rad(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor(Color::clear());
        return rv;
    }
}

class osc::LOGLPointShadowsTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
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
        App::upd().makeMainEventLoopWaiting();
        App::upd().setShowCursor(true);
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
        // move light position over time
        double const seconds = App::get().getFrameDeltaSinceAppStartup().count();
        m_LightPos.x = static_cast<float>(3.0 * std::sin(0.5 * seconds));
    }

    void implOnDraw() final
    {
        handleMouseCapture();
        draw3DScene();
        draw2DUI();
    }

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
        Rect const viewportRect = GetMainViewportWorkspaceScreenRect();

        drawShadowPassToCubemap();
        drawShadowmappedSceneToScreen(viewportRect);
    }

    void drawShadowPassToCubemap()
    {
        // create a 90 degree cube cone projection matrix
        float const nearPlane = 0.1f;
        float const farPlane = 25.0f;
        Mat4 const projectionMatrix = Perspective(
            Deg2Rad(90.0f),
            AspectRatio(c_ShadowmapDims),
            nearPlane,
            farPlane
        );

        // have the cone point toward all 6 faces of the cube
        auto const shadowMatrices =
            osc::CalcCubemapViewProjMatrices(projectionMatrix, m_LightPos);

        // pass data to material
        m_ShadowMappingMaterial.setMat4Array("uShadowMatrices", shadowMatrices);
        m_ShadowMappingMaterial.setVec3("uLightPos", m_LightPos);
        m_ShadowMappingMaterial.setFloat("uFarPlane", farPlane);

        // render (shadowmapping does not use the camera's view/projection matrices)
        Camera camera;
        for (SceneCube const& sceneCube : m_SceneCubes)
        {
            Graphics::DrawMesh(m_CubeMesh, sceneCube.transform, m_ShadowMappingMaterial, camera);
        }
        camera.renderTo(m_DepthTexture);
    }

    void drawShadowmappedSceneToScreen(Rect const& viewportRect)
    {
        Material material = m_UseSoftShadows ? m_SoftSceneMaterial : m_SceneMaterial;

        // set shared material params
        material.setTexture("uDiffuseTexture", m_WoodTexture);
        material.setVec3("uLightPos", m_LightPos);
        material.setVec3("uViewPos", m_SceneCamera.getPosition());
        material.setFloat("uFarPlane", 25.0f);
        material.setBool("uShadows", m_ShowShadows);

        for (SceneCube const& cube : m_SceneCubes)
        {
            MaterialPropertyBlock mpb;
            mpb.setBool("uReverseNormals", cube.invertNormals);
            material.setRenderTexture("uDepthMap", m_DepthTexture);
            Graphics::DrawMesh(m_CubeMesh, cube.transform, material, m_SceneCamera, std::move(mpb));
            material.clearRenderTexture("uDepthMap");
        }

        // also, draw the light as a little cube
        {
            material.setBool("uShadows", m_ShowShadows);  // always render shadows
            Transform t;
            t.scale *= 0.1f;
            t.position = m_LightPos;
            Graphics::DrawMesh(m_CubeMesh, t, material, m_SceneCamera);
        }

        m_SceneCamera.setPixelRect(viewportRect);
        m_SceneCamera.renderToScreen();
        m_SceneCamera.setPixelRect(std::nullopt);
    }

    void draw2DUI()
    {
        ImGui::Begin("controls");
        ImGui::Checkbox("show shadows", &m_ShowShadows);
        ImGui::Checkbox("soften shadows", &m_UseSoftShadows);
        ImGui::End();

        m_PerfPanel.onDraw();
    }

    Material m_ShadowMappingMaterial
    {
        Shader
        {
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/MakeShadowMap.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/MakeShadowMap.geom"),
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/MakeShadowMap.frag"),
        },
    };
    Material m_SceneMaterial
    {
        Shader
        {
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/Scene.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/Scene.frag"),
        },
    };

    Material m_SoftSceneMaterial
    {
        Shader
        {
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/Scene.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/SoftScene.frag"),
        }
    };

    Camera m_SceneCamera = CreateCamera();
    Vec3 m_CameraEulers = {};
    Texture2D m_WoodTexture = LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/wood.png"),
        ColorSpace::sRGB
    );
    Mesh m_CubeMesh = GenCube();
    std::array<SceneCube, 6> m_SceneCubes = MakeSceneCubes();
    RenderTexture m_DepthTexture = CreateDepthTexture();
    Vec3 m_LightPos = {};
    bool m_IsMouseCaptured = false;
    bool m_ShowShadows = true;
    bool m_UseSoftShadows = false;

    PerfPanel m_PerfPanel{"Perf"};
};


// public API

CStringView osc::LOGLPointShadowsTab::id()
{
    return c_TabStringID;
}

osc::LOGLPointShadowsTab::LOGLPointShadowsTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPointShadowsTab::LOGLPointShadowsTab(LOGLPointShadowsTab&&) noexcept = default;
osc::LOGLPointShadowsTab& osc::LOGLPointShadowsTab::operator=(LOGLPointShadowsTab&&) noexcept = default;
osc::LOGLPointShadowsTab::~LOGLPointShadowsTab() noexcept = default;

osc::UID osc::LOGLPointShadowsTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLPointShadowsTab::implGetName() const
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
