#include "LOGLShadowMappingTab.hpp"

#include <oscar_learnopengl/MouseCapturingCamera.hpp>

#include <SDL_events.h>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/RenderTexture.hpp>
#include <oscar/Graphics/RenderTextureDescriptor.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/UnitVec3.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <cstdint>
#include <memory>
#include <optional>

using namespace osc::literals;
using osc::CStringView;
using osc::Mesh;
using osc::MouseCapturingCamera;
using osc::RenderTexture;
using osc::RenderTextureDescriptor;
using osc::RenderTextureReadWrite;
using osc::Vec2;
using osc::Vec2i;
using osc::Vec3;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/ShadowMapping";

    // this matches the plane vertices used in the LearnOpenGL tutorial
    Mesh GeneratePlaneMesh()
    {
        Mesh rv;
        rv.setVerts({{
            { 25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f, -25.0f},

            { 25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f, -25.0f},
            { 25.0f, -0.5f, -25.0f},
        }});
        rv.setNormals({{
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},

            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
        }});
        rv.setTexCoords({{
            {25.0f,  0.0f},
            {0.0f,  0.0f},
            {0.0f, 25.0f},

            {25.0f,  0.0f},
            {0.0f, 25.0f},
            {25.0f, 25.0f},
        }});
        rv.setIndices(std::to_array<uint16_t>({
            0, 1, 2, 3, 4, 5
        }));
        return rv;
    }

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera cam;
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
        Rect const viewportRect = GetMainViewportWorkspaceScreenRect();

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
        Graphics::BlitToScreen(m_DepthTexture, Rect{viewportRect.p1, viewportRect.p1 + 200.0f});

        m_SceneMaterial.clearRenderTexture("uShadowMapTexture");
    }

    void drawMeshesWithMaterial(Material const& material)
    {
        // floor
        Graphics::DrawMesh(m_PlaneMesh, Identity<Transform>(), material, m_Camera);

        // cubes
        Graphics::DrawMesh(
            m_CubeMesh,
            {.scale = Vec3{0.5f}, .position = {0.0f, 1.0f, 0.0f}},
            material,
            m_Camera
        );
        Graphics::DrawMesh(
            m_CubeMesh,
            {.scale = Vec3{0.5f}, .position = {2.0f, 0.0f, 1.0f}},
            material,
            m_Camera
        );
        Graphics::DrawMesh(
            m_CubeMesh,
            Transform{
                .scale = Vec3{0.25f},
                .rotation = AngleAxis(60_deg, UnitVec3{1.0f, 0.0f, 1.0f}),
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
        Mat4 const lightViewMatrix = LookAt(m_LightPos, Vec3{0.0f}, {0.0f, 1.0f, 0.0f});
        Mat4 const lightProjMatrix = Ortho(-10.0f, 10.0f, -10.0f, 10.0f, zNear, zFar);
        m_LatestLightSpaceMatrix = lightProjMatrix * lightViewMatrix;

        drawMeshesWithMaterial(m_DepthMaterial);

        m_Camera.setViewMatrixOverride(lightViewMatrix);
        m_Camera.setProjectionMatrixOverride(lightProjMatrix);
        m_Camera.renderTo(m_DepthTexture);
        m_Camera.setViewMatrixOverride(std::nullopt);
        m_Camera.setProjectionMatrixOverride(std::nullopt);
    }

    MouseCapturingCamera m_Camera = CreateCamera();
    Texture2D m_WoodTexture = LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/wood.png"),
        ColorSpace::sRGB
    );
    Mesh m_CubeMesh = GenerateCubeMesh();
    Mesh m_PlaneMesh = GeneratePlaneMesh();
    Material m_SceneMaterial{Shader{
        App::slurp("oscar_learnopengl/shaders/AdvancedLighting/shadow_mapping/Scene.vert"),
        App::slurp("oscar_learnopengl/shaders/AdvancedLighting/shadow_mapping/Scene.frag"),
    }};
    Material m_DepthMaterial{Shader{
        App::slurp("oscar_learnopengl/shaders/AdvancedLighting/shadow_mapping/MakeShadowMap.vert"),
        App::slurp("oscar_learnopengl/shaders/AdvancedLighting/shadow_mapping/MakeShadowMap.frag"),
    }};
    RenderTexture m_DepthTexture = CreateDepthTexture();
    Mat4 m_LatestLightSpaceMatrix = Identity<Mat4>();
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

osc::UID osc::LOGLShadowMappingTab::implGetID() const
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
