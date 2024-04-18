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

    // returns random 3D decorations for the scene
    std::vector<TransformedMesh> generateDecorations()
    {
        auto const possibleGeometries = std::to_array<Mesh>({
            SphereGeometry{},
            TorusKnotGeometry{},
            IcosahedronGeometry{},
            BoxGeometry{},
        });

        auto rng = std::default_random_engine{std::random_device{}()};
        auto dist = std::normal_distribution{0.1f, 0.2f};
        AABB const gridBounds = {{-5.0f,  0.0f, -5.0f}, {5.0f, 0.0f, 5.0f}};
        Vec3 const gridDims = dimensions_of(gridBounds);
        Vec2uz const gridCells = {10, 10};

        std::vector<TransformedMesh> rv;
        rv.reserve(gridCells.x * gridCells.y);

        for (size_t x = 0; x < gridCells.x; ++x) {
            for (size_t y = 0; y < gridCells.y; ++y) {

                Vec3 const cellPos = gridBounds.min + gridDims * (Vec3{x, 0.0f, y} / Vec3{gridCells.x - 1_uz, 1, gridCells.y - 1_uz});
                Mesh mesh;
                sample(possibleGeometries, &mesh, 1, rng);

                rv.push_back(TransformedMesh{
                    .mesh = mesh,
                    .transform = {
                        .scale = Vec3{abs(dist(rng))},
                        .position = cellPos,
                    }
                });
            }
        }

        return rv;
    }

    // represents the 8 corners of a view frustum
    using FrustumCorners = std::array<Vec3, 8>;

    // represents orthogonal projection parameters
    struct OrthogonalProjectionParameters final {
        float r = quiet_nan_v<float>;
        float l = quiet_nan_v<float>;
        float b = quiet_nan_v<float>;
        float t = quiet_nan_v<float>;
        float f = quiet_nan_v<float>;
        float n = quiet_nan_v<float>;
    };

    // returns orthogonal projection information for each cascade
    std::vector<OrthogonalProjectionParameters> CalcOrthoProjections(
        Camera const& camera,
        float aspectRatio,
        UnitVec3 lightDirection)
    {
        // most of the maths/logic here was ported from an excellently-written ogldev tutorial:
        //
        // - https://ogldev.org/www/tutorial49/tutorial49.html

        // normalized means that 0.0 == near and 1.0 == far
        //
        // these planes are paired to figure out the near/far planes of each CSM's frustum
        constexpr auto normalizedCascadePlanes = std::to_array({ 0.0f, 1.0f/3.0f, 2.0f/3.0f, 3.0f/3.0f });

        // precompure transforms
        Mat4 const model2light = look_at({0.0f, 0.0f, 0.0f}, Vec3{lightDirection}, {0.0f, 1.0f, 0.0f});
        Mat4 const view2model = inverse(camera.view_matrix());
        Mat4 const view2light = model2light * view2model;

        // precompute necessary values to figure out the corners of the view frustum
        float const viewZNear = camera.near_clipping_plane();
        float const viewZFar = camera.far_clipping_plane();
        Radians const viewVFOV = camera.vertical_fov();
        Radians const viewHFOV = vertial_to_horizontal_fov(viewVFOV, aspectRatio);
        float const viewTanHalfVFOV = tan(0.5f * viewVFOV);
        float const viewTanHalfHFOV = tan(0.5f * viewHFOV);

        // calculate `OrthogonalProjectionParameters` for each cascade
        std::vector<OrthogonalProjectionParameters> rv;
        rv.reserve(normalizedCascadePlanes.size() - 1);
        for (size_t i = 0; i < normalizedCascadePlanes.size()-1; ++i) {
            float const viewCascadeZNear = lerp(viewZNear, viewZFar, normalizedCascadePlanes[i]);
            float const viewCascadeZFar = lerp(viewZNear, viewZFar, normalizedCascadePlanes[i+1]);

            // imagine a triangle with a point where the viewer is (0,0,0 in view-space) and another
            // point thats (e.g.) znear away from the viewer: the FOV dictates the angle of the corner
            // that originates from the viewer
            float const viewCascadeXNear = viewCascadeZNear * viewTanHalfHFOV;
            float const viewCascadeXFar  = viewCascadeZFar  * viewTanHalfHFOV;
            float const viewCascadeYNear = viewCascadeZNear * viewTanHalfVFOV;
            float const viewCascadeYFar  = viewCascadeZFar  * viewTanHalfVFOV;

            FrustumCorners const viewFrustumCorners = {
                // near face
                Vec3{ viewCascadeXNear,  viewCascadeYNear, viewCascadeZNear},  // top-right
                Vec3{-viewCascadeXNear,  viewCascadeYNear, viewCascadeZNear},  // top-left
                Vec3{ viewCascadeXNear, -viewCascadeYNear, viewCascadeZNear},  // bottom-right
                Vec3{-viewCascadeXNear, -viewCascadeYNear, viewCascadeZNear},  // bottom-left

                // far face
                Vec3{ viewCascadeXFar,  viewCascadeYFar, viewCascadeZFar},     // top-right
                Vec3{-viewCascadeXFar,  viewCascadeYFar, viewCascadeZFar},     // top-left
                Vec3{ viewCascadeXFar, -viewCascadeYFar, viewCascadeZFar},     // bottom-right
                Vec3{-viewCascadeXFar, -viewCascadeYFar, viewCascadeZFar},     // bottom-left
            };

            // compute the bounds in light-space by projecting each corner into light-space and min-maxing
            Vec3 lightBoundsMin = transform_point(view2light, viewFrustumCorners.front());
            Vec3 lightBoundsMax = lightBoundsMin;
            for (size_t corner = 1; corner < viewFrustumCorners.size(); ++corner) {
                Vec3 const lightCorner = transform_point(view2light, viewFrustumCorners[corner]);
                lightBoundsMin = elementwise_min(lightBoundsMin, lightCorner);
                lightBoundsMax = elementwise_max(lightBoundsMax, lightCorner);
            }

            // then use those bounds to compure the orthogonal projection parameters of
            // the directional light
            rv.push_back(OrthogonalProjectionParameters{
                .r = lightBoundsMax.x,
                .l = lightBoundsMin.x,
                .b = lightBoundsMin.y,
                .t = lightBoundsMax.y,
                .f = lightBoundsMax.z,
                .n = lightBoundsMin.z,
            });
        }
        return rv;
    }
}

class osc::LOGLCSMTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_UserCamera.set_near_clipping_plane(0.1f);
        m_UserCamera.set_far_clipping_plane(100.0f);
        m_Material.set_light_position(Vec3{5.0f});
        m_Material.set_diffuse_color(Color::orange());
        m_Decorations.push_back(TransformedMesh{
            .mesh = PlaneGeometry{},
            .transform = {
                .scale = Vec3{10.0f, 10.0f, 1.0f},
                .rotation = angle_axis(-90_deg, CoordinateDirection::x()),
                .position = {0.0f, -1.0f, 0.0f},
            },
        });
    }

private:
    void implOnMount() final
    {
        App::upd().make_main_loop_polling();
        m_UserCamera.onMount();
    }

    void implOnUnmount() final
    {
        m_UserCamera.onUnmount();
        App::upd().make_main_loop_waiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_UserCamera.onEvent(e);
    }

    void implOnDraw() final
    {
        m_UserCamera.onDraw();  // update from inputs etc.
        m_Material.set_viewer_position(m_UserCamera.position());

        for (auto const& decoration : m_Decorations) {
            graphics::draw(decoration.mesh, decoration.transform, m_Material, m_UserCamera);
        }

        m_UserCamera.set_pixel_rect(ui::GetMainViewportWorkspaceScreenRect());
        m_UserCamera.render_to_screen();
    }

    void drawShadowmaps()
    {
        CalcOrthoProjections(m_UserCamera, 1.0f, UnitVec3{0.0f, -1.0f, 0.0f});  // TODO
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
