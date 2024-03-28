#include "LOGLLightingMapsTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/LightingMaps";

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        return rv;
    }

    Material CreateLightMappingMaterial(IResourceLoader& rl)
    {
        Texture2D diffuseMap = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/container2.png"),
            ColorSpace::sRGB,
            ImageLoadingFlags::FlipVertically
        );

        Texture2D specularMap = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/container2_specular.png"),
            ColorSpace::sRGB,
            ImageLoadingFlags::FlipVertically
        );

        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/Lighting/LightingMaps.vert"),
            rl.slurp("oscar_learnopengl/shaders/Lighting/LightingMaps.frag"),
        }};
        rv.setTexture("uMaterialDiffuse", diffuseMap);
        rv.setTexture("uMaterialSpecular", specularMap);
        return rv;
    }
}

class osc::LOGLLightingMapsTab::Impl final : public StandardTabImpl {
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

        // clear screen and ensure camera has correct pixel rect
        App::upd().clear_screen({0.1f, 0.1f, 0.1f, 1.0f});

        // draw cube
        m_LightingMapsMaterial.setVec3("uViewPos", m_Camera.position());
        m_LightingMapsMaterial.setVec3("uLightPos", m_LightTransform.position);
        m_LightingMapsMaterial.setFloat("uLightAmbient", m_LightAmbient);
        m_LightingMapsMaterial.setFloat("uLightDiffuse", m_LightDiffuse);
        m_LightingMapsMaterial.setFloat("uLightSpecular", m_LightSpecular);
        m_LightingMapsMaterial.setFloat("uMaterialShininess", m_MaterialShininess);
        graphics::draw(m_Mesh, identity<Transform>(), m_LightingMapsMaterial, m_Camera);

        // draw lamp
        m_LightCubeMaterial.setColor("uLightColor", Color::white());
        graphics::draw(m_Mesh, m_LightTransform, m_LightCubeMaterial, m_Camera);

        // render 3D scene
        m_Camera.set_pixel_rect(ui::GetMainViewportWorkspaceScreenRect());
        m_Camera.render_to_screen();

        // render 2D UI
        ui::Begin("controls");
        ui::InputVec3("uLightPos", m_LightTransform.position);
        ui::InputFloat("uLightAmbient", &m_LightAmbient);
        ui::InputFloat("uLightDiffuse", &m_LightDiffuse);
        ui::InputFloat("uLightSpecular", &m_LightSpecular);
        ui::InputFloat("uMaterialShininess", &m_MaterialShininess);
        ui::End();
    }

    ResourceLoader m_Loader = App::resource_loader();
    Material m_LightingMapsMaterial = CreateLightMappingMaterial(m_Loader);
    Material m_LightCubeMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/LightCube.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/LightCube.frag"),
    }};
    Mesh m_Mesh = BoxGeometry{};
    MouseCapturingCamera m_Camera = CreateCamera();

    Transform m_LightTransform = {
        .scale = Vec3{0.2f},
        .position = {0.4f, 0.4f, 2.0f},
    };
    float m_LightAmbient = 0.02f;
    float m_LightDiffuse = 0.4f;
    float m_LightSpecular = 1.0f;
    float m_MaterialShininess = 64.0f;
};


// public API

CStringView osc::LOGLLightingMapsTab::id()
{
    return c_TabStringID;
}

osc::LOGLLightingMapsTab::LOGLLightingMapsTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLLightingMapsTab::LOGLLightingMapsTab(LOGLLightingMapsTab&&) noexcept = default;
osc::LOGLLightingMapsTab& osc::LOGLLightingMapsTab::operator=(LOGLLightingMapsTab&&) noexcept = default;
osc::LOGLLightingMapsTab::~LOGLLightingMapsTab() noexcept = default;

UID osc::LOGLLightingMapsTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLLightingMapsTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLLightingMapsTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLLightingMapsTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLLightingMapsTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLLightingMapsTab::implOnDraw()
{
    m_Impl->onDraw();
}
