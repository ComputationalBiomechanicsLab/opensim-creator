#include "LOGLPointShadowsTab.h"

#include <oscar_learnopengl/MouseCapturingCamera.h>

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <cmath>
#include <array>
#include <chrono>
#include <utility>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr Vec2i c_ShadowmapDims = {1024, 1024};
    constexpr CStringView c_TabStringID = "LearnOpenGL/PointShadows";

    Transform MakeRotatedTransform()
    {
        return {
            .scale = Vec3(0.75f),
            .rotation = angle_axis(60_deg, UnitVec3{1.0f, 0.0f, 1.0f}),
            .position = {-1.5f, 2.0f, -3.0f},
        };
    }

    struct SceneCube final {
        explicit SceneCube(Transform transform_) :
            transform{transform_}
        {}

        SceneCube(Transform transform_, bool invertNormals_) :
            transform{transform_},
            invertNormals{invertNormals_}
        {}

        Transform transform;
        bool invertNormals = false;
    };

    auto MakeSceneCubes()
    {
        return std::to_array<SceneCube>({
            SceneCube{{.scale = Vec3{5.0f}}, true},
            SceneCube{{.scale = Vec3{0.50f}, .position = {4.0f, -3.5f, 0.0f}}},
            SceneCube{{.scale = Vec3{0.75f}, .position = {2.0f, 3.0f, 1.0f}}},
            SceneCube{{.scale = Vec3{0.50f}, .position = {-3.0f, -1.0f, 0.0f}}},
            SceneCube{{.scale = Vec3{0.50f}, .position = {-1.5f, 1.0f, 1.5f}}},
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

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 5.0f});
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor(Color::clear());
        return rv;
    }
}

class osc::LOGLPointShadowsTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_SceneCamera.onMount();
    }

    void implOnUnmount() final
    {
        m_SceneCamera.onUnmount();
        App::upd().makeMainEventLoopWaiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_SceneCamera.onEvent(e);
    }

    void implOnTick() final
    {
        // move light position over time
        double const seconds = App::get().getFrameDeltaSinceAppStartup().count();
        m_LightPos.x = static_cast<float>(3.0 * sin(0.5 * seconds));
    }

    void implOnDraw() final
    {
        m_SceneCamera.onDraw();
        draw3DScene();
        draw2DUI();
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
        Mat4 const projectionMatrix = perspective(
            90_deg,
            AspectRatio(c_ShadowmapDims),
            nearPlane,
            farPlane
        );

        // have the cone point toward all 6 faces of the cube
        auto const shadowMatrices =
            CalcCubemapViewProjMatrices(projectionMatrix, m_LightPos);

        // pass data to material
        m_ShadowMappingMaterial.setMat4Array("uShadowMatrices", shadowMatrices);
        m_ShadowMappingMaterial.setVec3("uLightPos", m_LightPos);
        m_ShadowMappingMaterial.setFloat("uFarPlane", farPlane);

        // render (shadowmapping does not use the camera's view/projection matrices)
        Camera camera;
        for (SceneCube const& cube : m_SceneCubes) {
            Graphics::DrawMesh(m_CubeMesh, cube.transform, m_ShadowMappingMaterial, camera);
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

        for (SceneCube const& cube : m_SceneCubes) {
            MaterialPropertyBlock mpb;
            mpb.setBool("uReverseNormals", cube.invertNormals);
            material.setRenderTexture("uDepthMap", m_DepthTexture);
            Graphics::DrawMesh(m_CubeMesh, cube.transform, material, m_SceneCamera, std::move(mpb));
            material.clearRenderTexture("uDepthMap");
        }

        // also, draw the light as a little cube
        material.setBool("uShadows", m_ShowShadows);
        Graphics::DrawMesh(m_CubeMesh, {.scale = Vec3{0.1f}, .position = m_LightPos}, material, m_SceneCamera);

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

    ResourceLoader m_Loader = App::resource_loader();

    Material m_ShadowMappingMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/MakeShadowMap.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/MakeShadowMap.geom"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/MakeShadowMap.frag"),
    }};

    Material m_SceneMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/Scene.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/Scene.frag"),
    }};

    Material m_SoftSceneMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/Scene.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/point_shadows/SoftScene.frag"),
    }};

    MouseCapturingCamera m_SceneCamera = CreateCamera();
    Texture2D m_WoodTexture = LoadTexture2DFromImage(
        m_Loader.open("oscar_learnopengl/textures/wood.png"),
        ColorSpace::sRGB
    );
    Mesh m_CubeMesh = GenerateCubeMesh();
    std::array<SceneCube, 6> m_SceneCubes = MakeSceneCubes();
    RenderTexture m_DepthTexture = CreateDepthTexture();
    Vec3 m_LightPos = {};
    bool m_ShowShadows = true;
    bool m_UseSoftShadows = false;

    PerfPanel m_PerfPanel{"Perf"};
};


// public API

CStringView osc::LOGLPointShadowsTab::id()
{
    return c_TabStringID;
}

osc::LOGLPointShadowsTab::LOGLPointShadowsTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPointShadowsTab::LOGLPointShadowsTab(LOGLPointShadowsTab&&) noexcept = default;
osc::LOGLPointShadowsTab& osc::LOGLPointShadowsTab::operator=(LOGLPointShadowsTab&&) noexcept = default;
osc::LOGLPointShadowsTab::~LOGLPointShadowsTab() noexcept = default;

UID osc::LOGLPointShadowsTab::implGetID() const
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
