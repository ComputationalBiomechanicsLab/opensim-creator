#include "LOGLParallaxMappingTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/ParallaxMapping";

    // matches the quad used in LearnOpenGL's parallax mapping tutorial
    Mesh GenerateQuad()
    {
        Mesh rv;
        rv.setVerts({
            {-1.0f,  1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            { 1.0f, -1.0f, 0.0f},
            { 1.0f,  1.0f, 0.0f},
        });
        rv.setNormals({
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
        });
        rv.setTexCoords({
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
        });
        rv.setIndices({
            0, 1, 2,
            0, 2, 3,
        });
        rv.recalculateTangents();
        return rv;
    }

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        return rv;
    }

    Material CreateParallaxMappingMaterial(IResourceLoader& rl)
    {
        Texture2D diffuseMap = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/bricks2.jpg"),
            ColorSpace::sRGB
        );
        Texture2D normalMap = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/bricks2_normal.jpg"),
            ColorSpace::Linear
        );
        Texture2D displacementMap = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/bricks2_disp.jpg"),
            ColorSpace::Linear
        );

        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/ParallaxMapping.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/ParallaxMapping.frag"),
        }};
        rv.set_texture("uDiffuseMap", diffuseMap);
        rv.set_texture("uNormalMap", normalMap);
        rv.set_texture("uDisplacementMap", displacementMap);
        rv.set_float("uHeightScale", 0.1f);
        return rv;
    }

    Material CreateLightCubeMaterial(IResourceLoader& rl)
    {
        return Material{Shader{
            rl.slurp("oscar_learnopengl/shaders/LightCube.vert"),
            rl.slurp("oscar_learnopengl/shaders/LightCube.frag"),
        }};
    }
}

class osc::LOGLParallaxMappingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnMount() final
    {
        m_Camera.onMount();
    }

    void implOnUnmount() final
    {
        m_Camera.onUnmount();
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

        // draw normal-mapped quad
        {
            m_ParallaxMappingMaterial.set_vec3("uLightWorldPos", m_LightTransform.position);
            m_ParallaxMappingMaterial.set_vec3("uViewWorldPos", m_Camera.position());
            m_ParallaxMappingMaterial.set_bool("uEnableMapping", m_IsMappingEnabled);
            graphics::draw(m_QuadMesh, m_QuadTransform, m_ParallaxMappingMaterial, m_Camera);
        }

        // draw light source cube
        {
            m_LightCubeMaterial.set_color("uLightColor", Color::white());
            graphics::draw(m_CubeMesh, m_LightTransform, m_LightCubeMaterial, m_Camera);
        }

        m_Camera.set_pixel_rect(ui::GetMainViewportWorkspaceScreenRect());
        m_Camera.render_to_screen();

        ui::Begin("controls");
        ui::Checkbox("normal mapping", &m_IsMappingEnabled);
        ui::End();
    }

    ResourceLoader m_Loader = App::resource_loader();

    // rendering state
    Material m_ParallaxMappingMaterial = CreateParallaxMappingMaterial(m_Loader);
    Material m_LightCubeMaterial = CreateLightCubeMaterial(m_Loader);
    Mesh m_CubeMesh = BoxGeometry{};
    Mesh m_QuadMesh = GenerateQuad();

    // scene state
    MouseCapturingCamera m_Camera = CreateCamera();
    Transform m_QuadTransform;
    Transform m_LightTransform = {
        .scale = Vec3{0.2f},
        .position = {0.5f, 1.0f, 0.3f},
    };
    bool m_IsMappingEnabled = true;
};


// public API

CStringView osc::LOGLParallaxMappingTab::id()
{
    return c_TabStringID;
}

osc::LOGLParallaxMappingTab::LOGLParallaxMappingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLParallaxMappingTab::LOGLParallaxMappingTab(LOGLParallaxMappingTab&&) noexcept = default;
osc::LOGLParallaxMappingTab& osc::LOGLParallaxMappingTab::operator=(LOGLParallaxMappingTab&&) noexcept = default;
osc::LOGLParallaxMappingTab::~LOGLParallaxMappingTab() noexcept = default;

UID osc::LOGLParallaxMappingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLParallaxMappingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLParallaxMappingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLParallaxMappingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLParallaxMappingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLParallaxMappingTab::implOnDraw()
{
    m_Impl->onDraw();
}
