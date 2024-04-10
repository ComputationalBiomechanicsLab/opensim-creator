#include "LOGLPointShadowsTab.h"

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
        rv.set_position({0.0f, 0.0f, 5.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        rv.set_background_color(Color::clear());
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
        Rect const viewportRect = ui::GetMainViewportWorkspaceScreenRect();

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
            aspect_ratio(c_ShadowmapDims),
            nearPlane,
            farPlane
        );

        // have the cone point toward all 6 faces of the cube
        auto const shadowMatrices =
            CalcCubemapViewProjMatrices(projectionMatrix, m_LightPos);

        // pass data to material
        m_ShadowMappingMaterial.set_mat4_array("uShadowMatrices", shadowMatrices);
        m_ShadowMappingMaterial.set_vec3("uLightPos", m_LightPos);
        m_ShadowMappingMaterial.set_float("uFarPlane", farPlane);

        // render (shadowmapping does not use the camera's view/projection matrices)
        Camera camera;
        for (SceneCube const& cube : m_SceneCubes) {
            graphics::draw(m_CubeMesh, cube.transform, m_ShadowMappingMaterial, camera);
        }
        camera.render_to(m_DepthTexture);
    }

    void drawShadowmappedSceneToScreen(Rect const& viewportRect)
    {
        Material material = m_UseSoftShadows ? m_SoftSceneMaterial : m_SceneMaterial;

        // set shared material params
        material.set_texture("uDiffuseTexture", m_WoodTexture);
        material.set_vec3("uLightPos", m_LightPos);
        material.set_vec3("uViewPos", m_SceneCamera.position());
        material.set_float("uFarPlane", 25.0f);
        material.set_bool("uShadows", m_ShowShadows);

        for (SceneCube const& cube : m_SceneCubes) {
            MaterialPropertyBlock mpb;
            mpb.set_bool("uReverseNormals", cube.invertNormals);
            material.set_render_texture("uDepthMap", m_DepthTexture);
            graphics::draw(m_CubeMesh, cube.transform, material, m_SceneCamera, std::move(mpb));
            material.clear_render_texture("uDepthMap");
        }

        // also, draw the light as a little cube
        material.set_bool("uShadows", m_ShowShadows);
        graphics::draw(m_CubeMesh, {.scale = Vec3{0.1f}, .position = m_LightPos}, material, m_SceneCamera);

        m_SceneCamera.set_pixel_rect(viewportRect);
        m_SceneCamera.render_to_screen();
        m_SceneCamera.set_pixel_rect(std::nullopt);
    }

    void draw2DUI()
    {
        ui::Begin("controls");
        ui::Checkbox("show shadows", &m_ShowShadows);
        ui::Checkbox("soften shadows", &m_UseSoftShadows);
        ui::End();

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
    Texture2D m_WoodTexture = load_texture2D_from_image(
        m_Loader.open("oscar_learnopengl/textures/wood.png"),
        ColorSpace::sRGB
    );
    Mesh m_CubeMesh = BoxGeometry{2.0f, 2.0f, 2.0f};
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
