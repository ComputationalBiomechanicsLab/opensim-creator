#include "LOGLHDRTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_LightPositions = std::to_array<Vec3>({
        { 0.0f,  0.0f, 49.5f},
        {-1.4f, -1.9f, 9.0f},
        { 0.0f, -1.8f, 4.0f},
        { 0.8f, -1.7f, 6.0f},
    });

    constexpr CStringView c_TabStringID = "LearnOpenGL/HDR";

    std::array<Color, c_LightPositions.size()> GetLightColors()
    {
        return std::to_array<Color>({
            to_srgb_colorspace({200.0f, 200.0f, 200.0f, 1.0f}),
            to_srgb_colorspace({0.1f, 0.0f, 0.0f, 1.0f}),
            to_srgb_colorspace({0.0f, 0.0f, 0.2f, 1.0f}),
            to_srgb_colorspace({0.0f, 0.1f, 0.0f, 1.0f}),
        });
    }

    Transform CalcCorridoorTransform()
    {
        return {.scale = {2.5f, 2.5f, 27.5f}, .position = {0.0f, 0.0f, 25.0f}};
    }

    MouseCapturingCamera CreateSceneCamera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 5.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        rv.eulers() = {0_deg, 180_deg, 0_deg};
        return rv;
    }

    Material CreateSceneMaterial(IResourceLoader& rl)
    {
        Texture2D woodTexture = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/wood.png"),
            ColorSpace::sRGB
        );

        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Scene.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Scene.frag"),
        }};
        rv.set_vec3_array("uSceneLightPositions", c_LightPositions);
        rv.set_color_array("uSceneLightColors", GetLightColors());
        rv.set_texture("uDiffuseTexture", woodTexture);
        rv.set_bool("uInverseNormals", true);
        return rv;
    }

    Material CreateTonemapMaterial(IResourceLoader& rl)
    {
        return Material{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Tonemap.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Tonemap.frag"),
        }};
    }
}

class osc::LOGLHDRTab::Impl final : public StandardTabImpl {
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
        draw3DSceneToHDRTexture();
        drawHDRTextureViaTonemapperToScreen();
        draw2DUI();
    }

    void draw3DSceneToHDRTexture()
    {
        // reformat intermediate HDR texture to match tab dimensions etc.
        {
            Rect const viewportRect = ui::GetMainViewportWorkspaceScreenRect();
            RenderTextureDescriptor descriptor{dimensions_of(viewportRect)};
            descriptor.set_anti_aliasing_level(App::get().getCurrentAntiAliasingLevel());
            if (m_Use16BitFormat)
            {
                descriptor.set_color_format(RenderTextureFormat::ARGBFloat16);
            }

            m_SceneHDRTexture.reformat(descriptor);
        }

        graphics::draw(m_CubeMesh, m_CorridoorTransform, m_SceneMaterial, m_Camera);
        m_Camera.render_to(m_SceneHDRTexture);
    }

    void drawHDRTextureViaTonemapperToScreen()
    {
        Camera orthoCamera;
        orthoCamera.set_background_color(Color::clear());
        orthoCamera.set_pixel_rect(ui::GetMainViewportWorkspaceScreenRect());
        orthoCamera.set_projection_matrix_override(identity<Mat4>());
        orthoCamera.set_view_matrix_override(identity<Mat4>());

        m_TonemapMaterial.set_render_texture("uTexture", m_SceneHDRTexture);
        m_TonemapMaterial.set_bool("uUseTonemap", m_UseTonemap);
        m_TonemapMaterial.set_float("uExposure", m_Exposure);

        graphics::draw(m_QuadMesh, identity<Transform>(), m_TonemapMaterial, orthoCamera);
        orthoCamera.render_to_screen();

        m_TonemapMaterial.clear_render_texture("uTexture");
    }

    void draw2DUI()
    {
        ui::Begin("controls");
        ui::Checkbox("use tonemapping", &m_UseTonemap);
        ui::Checkbox("use 16-bit colors", &m_Use16BitFormat);
        ui::InputFloat("exposure", &m_Exposure);
        ui::Text("pos = %f,%f,%f", m_Camera.position().x, m_Camera.position().y, m_Camera.position().z);
        ui::Text("eulers = %f,%f,%f", m_Camera.eulers().x.count(), m_Camera.eulers().y.count(), m_Camera.eulers().z.count());
        ui::End();
    }

    ResourceLoader m_Loader = App::resource_loader();
    Material m_SceneMaterial = CreateSceneMaterial(m_Loader);
    Material m_TonemapMaterial = CreateTonemapMaterial(m_Loader);
    MouseCapturingCamera m_Camera = CreateSceneCamera();
    Mesh m_CubeMesh = BoxGeometry{2.0f, 2.0f, 2.0f};
    Mesh m_QuadMesh = PlaneGeometry{2.0f, 2.0f};
    Transform m_CorridoorTransform = CalcCorridoorTransform();
    RenderTexture m_SceneHDRTexture;
    float m_Exposure = 1.0f;
    bool m_Use16BitFormat = true;
    bool m_UseTonemap = true;
};


// public API

CStringView osc::LOGLHDRTab::id()
{
    return c_TabStringID;
}

osc::LOGLHDRTab::LOGLHDRTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLHDRTab::LOGLHDRTab(LOGLHDRTab&&) noexcept = default;
osc::LOGLHDRTab& osc::LOGLHDRTab::operator=(LOGLHDRTab&&) noexcept = default;
osc::LOGLHDRTab::~LOGLHDRTab() noexcept = default;

UID osc::LOGLHDRTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLHDRTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLHDRTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLHDRTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLHDRTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLHDRTab::implOnDraw()
{
    m_Impl->onDraw();
}
