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
        rv.set_vertices({
            { 25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f, -25.0f},

            { 25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f, -25.0f},
            { 25.0f, -0.5f, -25.0f},
        });
        rv.set_normals({
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},

            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
        });
        rv.set_tex_coords({
            {25.0f,  0.0f},
            {0.0f,  0.0f},
            {0.0f, 25.0f},

            {25.0f,  0.0f},
            {0.0f, 25.0f},
            {25.0f, 25.0f},
        });
        rv.set_indices({0, 1, 2, 3, 4, 5});
        return rv;
    }

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera cam;
        cam.set_position({-2.0f, 1.0f, 0.0f});
        cam.set_near_clipping_plane(0.1f);
        cam.set_far_clipping_plane(100.0f);
        return cam;
    }

    RenderTexture CreateDepthTexture()
    {
        RenderTexture rv;
        RenderTextureDescriptor shadowmapDescriptor{Vec2i{1024, 1024}};
        shadowmapDescriptor.set_read_write(RenderTextureReadWrite::Linear);
        rv.reformat(shadowmapDescriptor);
        return rv;
    }
}

class osc::LOGLShadowMappingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void impl_on_mount() final
    {
        App::upd().make_main_loop_polling();
        m_Camera.on_mount();
    }

    void impl_on_unmount() final
    {
        m_Camera.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool impl_on_event(SDL_Event const& e) final
    {
        return m_Camera.on_event(e);
    }

    void impl_on_draw() final
    {
        m_Camera.on_draw();
        draw3DScene();
    }

    void draw3DScene()
    {
        Rect const viewportRect = ui::get_main_viewport_workspace_screen_rect();

        renderShadowsToDepthTexture();

        m_Camera.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});

        m_SceneMaterial.set_vec3("uLightWorldPos", m_LightPos);
        m_SceneMaterial.set_vec3("uViewWorldPos", m_Camera.position());
        m_SceneMaterial.set_mat4("uLightSpaceMat", m_LatestLightSpaceMatrix);
        m_SceneMaterial.set_texture("uDiffuseTexture", m_WoodTexture);
        m_SceneMaterial.set_render_texture("uShadowMapTexture", m_DepthTexture);

        drawMeshesWithMaterial(m_SceneMaterial);
        m_Camera.set_pixel_rect(viewportRect);
        m_Camera.render_to_screen();
        m_Camera.set_pixel_rect(std::nullopt);
        graphics::blit_to_screen(m_DepthTexture, Rect{viewportRect.p1, viewportRect.p1 + 200.0f});

        m_SceneMaterial.clear_render_texture("uShadowMapTexture");
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

        m_Camera.set_view_matrix_override(lightViewMatrix);
        m_Camera.set_projection_matrix_override(lightProjMatrix);
        m_Camera.render_to(m_DepthTexture);
        m_Camera.set_view_matrix_override(std::nullopt);
        m_Camera.set_projection_matrix_override(std::nullopt);
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

UID osc::LOGLShadowMappingTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::LOGLShadowMappingTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::LOGLShadowMappingTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::LOGLShadowMappingTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::LOGLShadowMappingTab::impl_on_event(SDL_Event const& e)
{
    return m_Impl->on_event(e);
}

void osc::LOGLShadowMappingTab::impl_on_draw()
{
    m_Impl->on_draw();
}
