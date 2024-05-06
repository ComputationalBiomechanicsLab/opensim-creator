#include "LOGLFaceCullingTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/FaceCulling";

    Mesh GenerateCubeSimilarlyToLOGL()
    {
        return BoxGeometry{1.0f, 1.0f, 1.0f}.mesh();
    }

    Material GenerateUVTestingTextureMappedMaterial(IResourceLoader& rl)
    {
        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/FaceCulling.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/FaceCulling.frag"),
        }};

        rv.set_texture("uTexture", load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/uv_checker.jpg"),
            ColorSpace::sRGB
        ));

        return rv;
    }

    MouseCapturingCamera CreateCameraThatMatchesLearnOpenGL()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }
}

class osc::LOGLFaceCullingTab::Impl final : public StandardTabImpl {
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
        drawScene();
        draw2DUI();
    }

    void drawScene()
    {
        m_Camera.set_pixel_rect(ui::GetMainViewportWorkspaceScreenRect());
        graphics::draw(m_Cube, identity<Transform>(), m_Material, m_Camera);
        m_Camera.render_to_screen();
    }

    void draw2DUI()
    {
        ui::Begin("controls");
        if (ui::Button("off")) {
            m_Material.set_cull_mode(CullMode::Off);
        }
        if (ui::Button("back")) {
            m_Material.set_cull_mode(CullMode::Back);
        }
        if (ui::Button("front")) {
            m_Material.set_cull_mode(CullMode::Front);
        }
        ui::End();
    }

    ResourceLoader m_Loader = App::resource_loader();
    Material m_Material = GenerateUVTestingTextureMappedMaterial(m_Loader);
    Mesh m_Cube = GenerateCubeSimilarlyToLOGL();
    MouseCapturingCamera m_Camera = CreateCameraThatMatchesLearnOpenGL();
};


// public API

CStringView osc::LOGLFaceCullingTab::id()
{
    return c_TabStringID;
}

osc::LOGLFaceCullingTab::LOGLFaceCullingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLFaceCullingTab::LOGLFaceCullingTab(LOGLFaceCullingTab&&) noexcept = default;
osc::LOGLFaceCullingTab& osc::LOGLFaceCullingTab::operator=(LOGLFaceCullingTab&&) noexcept = default;
osc::LOGLFaceCullingTab::~LOGLFaceCullingTab() noexcept = default;

UID osc::LOGLFaceCullingTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::LOGLFaceCullingTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::LOGLFaceCullingTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::LOGLFaceCullingTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::LOGLFaceCullingTab::impl_on_event(SDL_Event const& e)
{
    return m_Impl->on_event(e);
}

void osc::LOGLFaceCullingTab::impl_on_draw()
{
    m_Impl->on_draw();
}
