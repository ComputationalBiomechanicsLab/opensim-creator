#include "MandelbrotTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <limits>
#include <memory>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "Demos/Mandelbrot";

    Camera CreateIdentityCamera()
    {
        Camera camera;
        camera.set_view_matrix_override(identity<Mat4>());
        camera.set_projection_matrix_override(identity<Mat4>());
        return camera;
    }
}

class osc::MandelbrotTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    bool impl_on_event(SDL_Event const& e) final
    {
        if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_PAGEUP && m_NumIterations < std::numeric_limits<decltype(m_NumIterations)>::max()) {
            m_NumIterations *= 2;
            return true;
        }
        if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_PAGEDOWN && m_NumIterations > 1) {
            m_NumIterations /= 2;
            return true;
        }
        if (e.type == SDL_MOUSEWHEEL) {
            float const factor = e.wheel.y > 0 ? 0.9f : 1.11111111f;

            applyZoom(ui::get_io().MousePos, factor);
            return true;
        }
        if (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK) != 0) {
            Vec2 const screenSpacePanAmount = {static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel)};
            applyPan(screenSpacePanAmount);
            return true;
        }
        return false;
    }

    void impl_on_draw() final
    {
        m_MainViewportWorkspaceScreenRect = ui::get_main_viewport_workspace_screen_rect();

        m_Material.set_vec2("uRescale", {1.0f, 1.0f});
        m_Material.set_vec2("uOffset", {});
        m_Material.set_int("uNumIterations", m_NumIterations);
        graphics::draw(m_QuadMesh, identity<Transform>(), m_Material, m_Camera);
        m_Camera.set_pixel_rect(m_MainViewportWorkspaceScreenRect);
        m_Camera.render_to_screen();
    }

    void applyZoom(Vec2, float)
    {
        // TODO: zoom the mandelbrot viewport into the given screen-space location by the given factor
    }

    void applyPan(Vec2)
    {
        // TODO: pan the mandelbrot viewport by the given screen-space offset vector
    }

    ResourceLoader m_Loader = App::resource_loader();
    int m_NumIterations = 16;
    Rect m_NormalizedMandelbrotViewport = {{}, {1.0f, 1.0f}};
    Rect m_MainViewportWorkspaceScreenRect = {};
    Mesh m_QuadMesh = PlaneGeometry{2.0f, 2.0f, 1, 1};
    Material m_Material{Shader{
        m_Loader.slurp("oscar_demos/shaders/Mandelbrot.vert"),
        m_Loader.slurp("oscar_demos/shaders/Mandelbrot.frag"),
    }};
    Camera m_Camera = CreateIdentityCamera();
};


// public API

CStringView osc::MandelbrotTab::id()
{
    return c_TabStringID;
}

osc::MandelbrotTab::MandelbrotTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{}

osc::MandelbrotTab::MandelbrotTab(MandelbrotTab&&) noexcept = default;
osc::MandelbrotTab& osc::MandelbrotTab::operator=(MandelbrotTab&&) noexcept = default;
osc::MandelbrotTab::~MandelbrotTab() noexcept = default;

UID osc::MandelbrotTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::MandelbrotTab::impl_get_name() const
{
    return m_Impl->name();
}

bool osc::MandelbrotTab::impl_on_event(SDL_Event const& e)
{
    return m_Impl->on_event(e);
}

void osc::MandelbrotTab::impl_on_draw()
{
    m_Impl->on_draw();
}
