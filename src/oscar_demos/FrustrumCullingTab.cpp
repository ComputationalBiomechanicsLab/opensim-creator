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
        Vec3 const dims = dimensions(bounds);
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

    struct Frustum final {
        std::array<Plane, 6> m_ClippingPlanes;
    };

    Frustum CalcCameraFrustum(Camera const& camera, float aspectRatio)
    {
        Radians const fovY = camera.getVerticalFOV();
        float const zNear = camera.getNearClippingPlane();
        float const zFar = camera.getFarClippingPlane();
        float const halfVSize = zFar * tan(fovY * 0.5f);
        float const halfHSize = halfVSize * aspectRatio;
        Vec3 const pos = camera.getPosition();
        Vec3 const front = camera.getDirection();
        Vec3 const up = camera.getUpwardsDirection();
        Vec3 const right = cross(front, up);
        Vec3 const frontMultnear = zNear * front;
        Vec3 const frontMultfar = zFar * front;

        return {
                  // origin           // normal
            Plane{pos + frontMultnear, -front},                                    // near
            Plane{pos + frontMultfar,   front},                                    // far
            Plane{pos               ,  -normalize(cross(frontMultfar - right*halfHSize, up))},  // right
            Plane{pos               ,  -normalize(cross(up, frontMultfar + right*halfHSize))},  // left
            Plane{pos               ,  -normalize(cross(right, frontMultfar - up*halfVSize))},  // top
            Plane{pos               ,  -normalize(cross(frontMultfar + up*halfVSize, right))},  // bottom
        };
    }

    // tests if `aabb` lies outside `frustum` (e.g. so cull it)
    bool is_inside_frustum(Frustum const& frustum, AABB const& aabb)
    {
        return !any_of(frustum.m_ClippingPlanes, [&aabb](Plane const& plane) { return is_in_front_of(plane, aabb); });
    }
}

class osc::FrustrumCullingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_UserCamera.setNearClippingPlane(0.1f);
        m_UserCamera.setFarClippingPlane(10.0f);
        m_TopDownCamera.setPosition({0.0f, 9.5f, 0.0f});
        m_TopDownCamera.setDirection({0.0f, -1.0f, 0.0f});
        m_TopDownCamera.setNearClippingPlane(0.1f);
        m_TopDownCamera.setFarClippingPlane(10.0f);
    }

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_UserCamera.onMount();
    }

    void implOnUnmount() final
    {
        m_UserCamera.onUnmount();
        App::upd().makeMainEventLoopWaiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_UserCamera.onEvent(e);
    }

    void implOnDraw() final
    {
        Rect const viewport = ui::GetMainViewportWorkspaceScreenRect();
        float const xmid = midpoint(viewport.p1.x, viewport.p2.x);
        Rect const lhs = {viewport.p1, {xmid, viewport.p2.y}};
        Rect const rhs = {{xmid, viewport.p1.y}, viewport.p2};
        Frustum const frustum = CalcCameraFrustum(m_UserCamera, AspectRatio(lhs));

        m_UserCamera.onDraw();  // update from inputs etc.

        // render from user's perspective on left-hand side
        for (auto const& dec : m_Decorations) {
            AABB const aabb = transform_aabb(dec.mesh.getBounds(), dec.transform);
            if (is_inside_frustum(frustum, aabb)) {
                Graphics::DrawMesh(dec.mesh, dec.transform, m_Material, m_UserCamera, m_BlueMaterialProps);
            }
        }
        m_UserCamera.setPixelRect(lhs);
        m_UserCamera.renderToScreen();

        // render from top-down perspective on right-hand side
        for (auto const& dec : m_Decorations) {
            AABB const aabb = transform_aabb(dec.mesh.getBounds(), dec.transform);
            auto const& props = is_inside_frustum(frustum, aabb) ? m_BlueMaterialProps : m_RedMaterialProps;
            Graphics::DrawMesh(dec.mesh, dec.transform, m_Material, m_TopDownCamera, props);
        }
        Graphics::DrawMesh(
            SphereGeometry{},
            {.scale = Vec3{0.1f}, .position = m_UserCamera.getPosition()},
            m_Material,
            m_TopDownCamera,
            m_GreenMaterialProps
        );
        m_TopDownCamera.setPixelRect(rhs);
        m_TopDownCamera.setScissorRect(rhs);
        m_TopDownCamera.setBackgroundColor({0.1f, 1.0f});
        m_TopDownCamera.renderToScreen();
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
