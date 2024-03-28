#include "LOGLShadowMappingTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <cstdint>
#include <memory>
#include <optional>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/ShadowMapping";

    // this matches the plane vertices used in the LearnOpenGL tutorial
    Mesh GeneratePlaneMeshLOGL()
    {
        Mesh rv;
        rv.setVerts({
            { 25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f, -25.0f},

            { 25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f, -25.0f},
            { 25.0f, -0.5f, -25.0f},
        });
        rv.setNormals({
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},

            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
        });
        rv.setTexCoords({
            {25.0f,  0.0f},
            {0.0f,  0.0f},
            {0.0f, 25.0f},

            {25.0f,  0.0f},
            {0.0f, 25.0f},
            {25.0f, 25.0f},
        });
        rv.setIndices({0, 1, 2, 3, 4, 5});
        return rv;
    }

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera cam;
        cam.setPosition({-2.0f, 1.0f, 0.0f});
        cam.setNearClippingPlane(0.1f);
        cam.setFarClippingPlane(100.0f);
        return cam;
    }

    RenderTexture CreateDepthTexture()
    {
        RenderTexture rv;
        RenderTextureDescriptor shadowmapDescriptor{Vec2i{1024, 1024}};
        shadowmapDescriptor.setReadWrite(RenderTextureReadWrite::Linear);
        rv.reformat(shadowmapDescriptor);
        return rv;
    }
}

class osc::LOGLShadowMappingTab::Impl final : public StandardTabImpl {
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

    void implOnDraw() final
    {
        m_Camera.onDraw();
        draw3DScene();
    }

    void draw3DScene()
    {
        Rect const viewportRect = ui::GetMainViewportWorkspaceScreenRect();

        renderShadowsToDepthTexture();

        m_Camera.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});

        m_SceneMaterial.setVec3("uLightWorldPos", m_LightPos);
        m_SceneMaterial.setVec3("uViewWorldPos", m_Camera.getPosition());
        m_SceneMaterial.setMat4("uLightSpaceMat", m_LatestLightSpaceMatrix);
        m_SceneMaterial.setTexture("uDiffuseTexture", m_WoodTexture);
        m_SceneMaterial.setRenderTexture("uShadowMapTexture", m_DepthTexture);

        drawMeshesWithMaterial(m_SceneMaterial);
        m_Camera.setPixelRect(viewportRect);
        m_Camera.renderToScreen();
        m_Camera.setPixelRect(std::nullopt);
        graphics::blitToScreen(m_DepthTexture, Rect{viewportRect.p1, viewportRect.p1 + 200.0f});

        m_SceneMaterial.clearRenderTexture("uShadowMapTexture");
    }

    void drawMeshesWithMaterial(Material const& material)
    {
        // floor
        graphics::draw(m_PlaneMesh, identity<Transform>(), material, m_Camera);

        // cubes
        graphics::draw(
            m_CubeMesh,
            {.scale = Vec3{0.5f}, .position = {0.0f, 1.0f, 0.0f}},
            material,
            m_Camera
        );
        graphics::draw(
            m_CubeMesh,
            {.scale = Vec3{0.5f}, .position = {2.0f, 0.0f, 1.0f}},
            material,
            m_Camera
        );
        graphics::draw(
            m_CubeMesh,
            Transform{
                .scale = Vec3{0.25f},
                .rotation = angle_axis(60_deg, UnitVec3{1.0f, 0.0f, 1.0f}),
                .position = {-1.0f, 0.0f, 2.0f},
            },
            material,
            m_Camera
        );
    }

    void renderShadowsToDepthTexture()
    {
        float const zNear = 1.0f;
        float const zFar = 7.5f;
        Mat4 const lightViewMatrix = look_at(m_LightPos, Vec3{0.0f}, {0.0f, 1.0f, 0.0f});
        Mat4 const lightProjMatrix = ortho(-10.0f, 10.0f, -10.0f, 10.0f, zNear, zFar);
        m_LatestLightSpaceMatrix = lightProjMatrix * lightViewMatrix;

        drawMeshesWithMaterial(m_DepthMaterial);

        m_Camera.setViewMatrixOverride(lightViewMatrix);
        m_Camera.setProjectionMatrixOverride(lightProjMatrix);
        m_Camera.renderTo(m_DepthTexture);
        m_Camera.setViewMatrixOverride(std::nullopt);
        m_Camera.setProjectionMatrixOverride(std::nullopt);
    }

    ResourceLoader m_Loader = App::resource_loader();
    MouseCapturingCamera m_Camera = CreateCamera();
    Texture2D m_WoodTexture = load_texture2D_from_image(
        m_Loader.open("oscar_learnopengl/textures/wood.png"),
        ColorSpace::sRGB
    );
    Mesh m_CubeMesh = BoxGeometry{2.0f, 2.0f, 2.0f};
    Mesh m_PlaneMesh = GeneratePlaneMeshLOGL();
    Material m_SceneMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/shadow_mapping/Scene.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/shadow_mapping/Scene.frag"),
    }};
    Material m_DepthMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/shadow_mapping/MakeShadowMap.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/shadow_mapping/MakeShadowMap.frag"),
    }};
    RenderTexture m_DepthTexture = CreateDepthTexture();
    Mat4 m_LatestLightSpaceMatrix = identity<Mat4>();
    Vec3 m_LightPos = {-2.0f, 4.0f, -1.0f};
};


// public API (PIMPL)

CStringView osc::LOGLShadowMappingTab::id()
{
    return c_TabStringID;
}

osc::LOGLShadowMappingTab::LOGLShadowMappingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLShadowMappingTab::LOGLShadowMappingTab(LOGLShadowMappingTab&&) noexcept = default;
osc::LOGLShadowMappingTab& osc::LOGLShadowMappingTab::operator=(LOGLShadowMappingTab&&) noexcept = default;
osc::LOGLShadowMappingTab::~LOGLShadowMappingTab() noexcept = default;

UID osc::LOGLShadowMappingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLShadowMappingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLShadowMappingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLShadowMappingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLShadowMappingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLShadowMappingTab::implOnDraw()
{
    m_Impl->onDraw();
}
