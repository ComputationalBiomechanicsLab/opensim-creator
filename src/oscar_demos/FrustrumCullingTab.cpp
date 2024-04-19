#include "FrustrumCullingTab.h"

#include <oscar/oscar.h>

#include <SDL_events.h>

#include <array>
#include <cstddef>
#include <memory>
#include <random>

using namespace osc;
using namespace osc::literals;

namespace
{
    constexpr CStringView c_TabStringID = "Demos/FrustrumCulling";

    struct TransformedMesh {
        Mesh mesh;
        Transform transform;
    };

    std::vector<TransformedMesh> generateDecorations()
    {
        auto const geoms = std::to_array<Mesh>({
            SphereGeometry{},
            TorusKnotGeometry{},
            IcosahedronGeometry{},
            BoxGeometry{},
        });

        auto rng = std::default_random_engine{std::random_device{}()};
        auto dist = std::normal_distribution{0.1f, 0.1f};
        AABB const bounds = {{-5.0f, -2.0f, -5.0f}, {5.0f, 2.0f, 5.0f}};
        Vec3 const dims = dimensions_of(bounds);
        Vec3uz const cells = {10, 3, 10};

        std::vector<TransformedMesh> rv;
        rv.reserve(cells.x * cells.y * cells.z);

        for (size_t x = 0; x < cells.x; ++x) {
            for (size_t y = 0; y < cells.y; ++y) {
                for (size_t z = 0; z < cells.z; ++z) {

                    Vec3 const pos = bounds.min + dims * (Vec3{x, y, z} / Vec3{cells - 1_uz});

                    Mesh mesh;
                    sample(geoms, &mesh, 1, rng);

                    rv.push_back(TransformedMesh{
                        .mesh = mesh,
                        .transform = {
                            .scale = Vec3{dist(rng)},
                            .position = pos,
                        }
                    });
                }
            }
        }

        return rv;
    }
}

class osc::FrustrumCullingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_UserCamera.set_near_clipping_plane(0.1f);
        m_UserCamera.set_far_clipping_plane(10.0f);
        m_TopDownCamera.set_position({0.0f, 9.5f, 0.0f});
        m_TopDownCamera.set_direction({0.0f, -1.0f, 0.0f});
        m_TopDownCamera.set_near_clipping_plane(0.1f);
        m_TopDownCamera.set_far_clipping_plane(10.0f);
    }

private:
    void implOnMount() final
    {
        App::upd().make_main_loop_polling();
        m_UserCamera.on_mount();
    }

    void implOnUnmount() final
    {
        m_UserCamera.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_UserCamera.on_event(e);
    }

    void implOnDraw() final
    {
        Rect const viewport = ui::GetMainViewportWorkspaceScreenRect();
        float const xmid = midpoint(viewport.p1.x, viewport.p2.x);
        Rect const lhs = {viewport.p1, {xmid, viewport.p2.y}};
        Rect const rhs = {{xmid, viewport.p1.y}, viewport.p2};
        FrustumPlanes const frustum = calc_frustum_planes(m_UserCamera, aspect_ratio(lhs));

        m_UserCamera.on_draw();  // update from inputs etc.

        // render from user's perspective on left-hand side
        for (auto const& dec : m_Decorations) {
            AABB const aabb = transform_aabb(dec.transform, dec.mesh.bounds());
            if (is_intersecting(frustum, aabb)) {
                graphics::draw(dec.mesh, dec.transform, m_Material, m_UserCamera, m_BlueMaterialProps);
            }
        }
        m_UserCamera.set_pixel_rect(lhs);
        m_UserCamera.render_to_screen();

        // render from top-down perspective on right-hand side
        for (auto const& dec : m_Decorations) {
            AABB const aabb = transform_aabb(dec.transform, dec.mesh.bounds());
            auto const& props = is_intersecting(frustum, aabb) ? m_BlueMaterialProps : m_RedMaterialProps;
            graphics::draw(dec.mesh, dec.transform, m_Material, m_TopDownCamera, props);
        }
        graphics::draw(
            SphereGeometry{},
            {.scale = Vec3{0.1f}, .position = m_UserCamera.position()},
            m_Material,
            m_TopDownCamera,
            m_GreenMaterialProps
        );
        m_TopDownCamera.set_pixel_rect(rhs);
        m_TopDownCamera.set_scissor_rect(rhs);
        m_TopDownCamera.set_background_color({0.1f, 1.0f});
        m_TopDownCamera.render_to_screen();
    }

    MouseCapturingCamera m_UserCamera;
    std::vector<TransformedMesh> m_Decorations = generateDecorations();
    Camera m_TopDownCamera;
    MeshBasicMaterial m_Material;
    MeshBasicMaterial::PropertyBlock m_RedMaterialProps{Color::red()};
    MeshBasicMaterial::PropertyBlock m_BlueMaterialProps{Color::blue()};
    MeshBasicMaterial::PropertyBlock m_GreenMaterialProps{Color::green()};
};


// public API

osc::CStringView osc::FrustrumCullingTab::id()
{
    return c_TabStringID;
}

osc::FrustrumCullingTab::FrustrumCullingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::FrustrumCullingTab::FrustrumCullingTab(FrustrumCullingTab&&) noexcept = default;
osc::FrustrumCullingTab& osc::FrustrumCullingTab::operator=(FrustrumCullingTab&&) noexcept = default;
osc::FrustrumCullingTab::~FrustrumCullingTab() noexcept = default;

UID osc::FrustrumCullingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::FrustrumCullingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::FrustrumCullingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::FrustrumCullingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::FrustrumCullingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::FrustrumCullingTab::implOnDraw()
{
    m_Impl->onDraw();
}
