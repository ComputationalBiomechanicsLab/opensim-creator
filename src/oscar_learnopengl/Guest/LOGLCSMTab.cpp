#include "LOGLCSMTab.h"

#include <oscar/oscar.h>

#include <SDL_events.h>

#include <array>
#include <cstddef>
#include <memory>
#include <random>
#include <vector>

using namespace osc;
using namespace osc::literals;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/CSM";

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
        auto dist = std::normal_distribution{0.1f, 0.2f};
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
                            .scale = Vec3{abs(dist(rng))},
                            .position = pos,
                        }
                    });
                }
            }
        }

        return rv;
    }
}

class osc::LOGLCSMTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_UserCamera.setNearClippingPlane(0.1f);
        m_UserCamera.setFarClippingPlane(100.0f);
        m_Material.setLightPosition(Vec3{5.0f});
        m_Material.setDiffuseColor(Color::orange());
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
        m_UserCamera.onDraw();  // update from inputs etc.
        m_Material.setViewerPosition(m_UserCamera.getPosition());

        for (auto const& decoration : m_Decorations) {
            Graphics::DrawMesh(decoration.mesh, decoration.transform, m_Material, m_UserCamera);
        }
        m_UserCamera.setPixelRect(ui::GetMainViewportWorkspaceScreenRect());
        m_UserCamera.renderToScreen();
    }

    MouseCapturingCamera m_UserCamera;
    std::vector<TransformedMesh> m_Decorations = generateDecorations();
    MeshPhongMaterial m_Material;
};


// public API

osc::CStringView osc::LOGLCSMTab::id()
{
    return c_TabStringID;
}

osc::LOGLCSMTab::LOGLCSMTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLCSMTab::LOGLCSMTab(LOGLCSMTab&&) noexcept = default;
osc::LOGLCSMTab& osc::LOGLCSMTab::operator=(LOGLCSMTab&&) noexcept = default;
osc::LOGLCSMTab::~LOGLCSMTab() noexcept = default;

UID osc::LOGLCSMTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLCSMTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLCSMTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLCSMTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLCSMTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLCSMTab::implOnDraw()
{
    m_Impl->onDraw();
}
