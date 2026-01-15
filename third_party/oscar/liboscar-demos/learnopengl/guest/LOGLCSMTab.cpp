#include "LOGLCSMTab.h"

#include <liboscar/graphics/Camera.h>
#include <liboscar/graphics/DepthStencilRenderBufferParams.h>
#include <liboscar/graphics/Graphics.h>
#include <liboscar/graphics/Material.h>
#include <liboscar/graphics/Mesh.h>
#include <liboscar/graphics/SharedDepthStencilRenderBuffer.h>
#include <liboscar/graphics/geometries/BoxGeometry.h>
#include <liboscar/graphics/geometries/IcosahedronGeometry.h>
#include <liboscar/graphics/geometries/PlaneGeometry.h>
#include <liboscar/graphics/geometries/SphereGeometry.h>
#include <liboscar/graphics/geometries/TorusKnotGeometry.h>
#include <liboscar/graphics/materials/MeshDepthWritingMaterial.h>
#include <liboscar/maths/AABBFunctions.h>
#include <liboscar/maths/MathHelpers.h>
#include <liboscar/maths/MatrixFunctions.h>
#include <liboscar/maths/QuaternionFunctions.h>
#include <liboscar/platform/app.h>
#include <liboscar/ui/mouse_capturing_camera.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/log_viewer_panel.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/utils/Assertions.h>

#include <array>
#include <cstddef>
#include <memory>
#include <random>
#include <ranges>
#include <span>
#include <vector>

using namespace osc;
using namespace osc::literals;
namespace rgs = std::ranges;

namespace
{
    constexpr int c_shadow_map_edge_length = 256;

    // represents a single transformed mesh in the scene
    struct TransformedMesh {
        Mesh mesh;
        Transform transform;
    };

    // returns random 3D decorations for the scene
    std::vector<TransformedMesh> generate_decorations()
    {
        const auto possible_geometries = std::to_array<Mesh>({
            SphereGeometry{},
            TorusKnotGeometry{},
            IcosahedronGeometry{},
            BoxGeometry{},
        });

        auto rng = std::default_random_engine{std::random_device{}()};
        auto dist = std::normal_distribution{0.3f, 0.2f};
        const AABB grid_bounds = {{-5.0f,  0.0f, -5.0f}, {5.0f, 0.0f, 5.0f}};
        const Vector3 grid_dimensions = dimensions_of(grid_bounds);
        const Vector2uz num_grid_cells = {10, 10};

        std::vector<TransformedMesh> rv;
        rv.reserve(num_grid_cells.x * num_grid_cells.y);

        for (size_t x = 0; x < num_grid_cells.x; ++x) {
            for (size_t y = 0; y < num_grid_cells.y; ++y) {

                const Vector3 cell_pos = grid_bounds.min + grid_dimensions * (Vector3{x, 0.0f, y} / Vector3{num_grid_cells.x - 1uz, 1, num_grid_cells.y - 1uz});
                Mesh mesh;
                rgs::sample(possible_geometries, &mesh, 1, rng);

                rv.push_back(TransformedMesh{
                    .mesh = mesh,
                    .transform = {
                        .scale = Vector3{abs(dist(rng))},
                        .translation = cell_pos,
                    }
                });
            }
        }

        // also, add a floor plane
        rv.push_back(TransformedMesh{
            .mesh = PlaneGeometry{},
            .transform = {
                .scale = {10.0f, 10.0f, 1.0f},
                .rotation = angle_axis(-90_deg, CoordinateDirection::x()),
                .translation = {0.0f, -1.0f, 0.0f},
            },
        });

        return rv;
    }

    // returns blank depth buffers that the cascade (shadow maps) are written to
    std::vector<SharedDepthStencilRenderBuffer> generate_blank_cascade_buffers()
    {
        const DepthStencilRenderBufferParams params = {
            .pixel_dimensions = Vector2i{c_shadow_map_edge_length},
            .format = DepthStencilRenderBufferFormat::D32_SFloat,
        };
        return {
            SharedDepthStencilRenderBuffer{params},
            SharedDepthStencilRenderBuffer{params},
            SharedDepthStencilRenderBuffer{params},
        };
    }

    // represents orthogonal projection parameters
    struct OrthogonalProjectionParameters final {
        float left = quiet_nan_v<float>;
        float right = quiet_nan_v<float>;
        float bottom = quiet_nan_v<float>;
        float top = quiet_nan_v<float>;
        float near = quiet_nan_v<float>;
        float far = quiet_nan_v<float>;
    };

    // the distance of each plane (incl. the near place) as a normalized range [0.0f, 1.0f], where
    // 0.0f means `znear` and 1.0f means `zfar`.
    constexpr auto c_normalized_cascade_planes = std::to_array({0.0f, 10.0f/100.0f, 50.0f/100.0f, 100.0f/100.0f});

    // returns orthogonal projection information for each cascade
    std::vector<OrthogonalProjectionParameters> calculate_light_source_orthographic_projections(
        const Camera& camera,
        float aspect_ratio,
        Vector3 light_world_direction)
    {
        // most of the maths/logic here was ported from an excellently-written ogldev tutorial:
        //
        // - https://ogldev.org/www/tutorial49/tutorial49.html

        // precompute transforms
        const Matrix4x4 world_to_light = look_at({0.0f, 0.0f, 0.0f}, Vector3{light_world_direction}, {0.0f, 1.0f, 0.0f});
        const Matrix4x4 view_to_world = camera.inverse_view_matrix();
        const Matrix4x4 view_to_light = world_to_light * view_to_world;

        // precompute necessary values to figure out the corners of the view frustum
        const auto [view_znear, view_zfar] = camera.clipping_planes();
        const Radians view_vfov = camera.vertical_field_of_view();
        const Radians view_hfov = camera.horizontal_field_of_view(aspect_ratio);
        const float view_tan_half_vfov = tan(0.5f * view_vfov);
        const float view_tan_half_hfov = tan(0.5f * view_hfov);

        // calculate `OrthogonalProjectionParameters` for each cascade
        std::vector<OrthogonalProjectionParameters> rv;
        rv.reserve(c_normalized_cascade_planes.size() - 1);
        for (size_t i = 0; i < c_normalized_cascade_planes.size()-1; ++i) {
            const float view_cascade_znear = lerp(view_znear, view_zfar, c_normalized_cascade_planes[i]);
            const float view_cascade_zfar = lerp(view_znear, view_zfar, c_normalized_cascade_planes[i+1]);

            // imagine a triangle with a point where the viewer is (0,0,0 in view space) and another
            // point thats znear along the minus Z axis (i.e. moving away from the front of the viewer
            // in a right-handed coordinate system). The FOV dictates the angle of the corner
            // that originates from the viewer.
            const float view_cascade_xnear = view_cascade_znear * view_tan_half_hfov;
            const float view_cascade_xfar  = view_cascade_zfar  * view_tan_half_hfov;
            const float view_cascade_ynear = view_cascade_znear * view_tan_half_vfov;
            const float view_cascade_yfar  = view_cascade_zfar  * view_tan_half_vfov;

            // note: Z points opposite to the viewing direction in a right-handed system, so we negate
            // all the Zs here.
            const auto view_frustum_corners = std::to_array<Vector3>({
                // near face
                { view_cascade_xnear,  view_cascade_ynear, -view_cascade_znear},  // top-right
                {-view_cascade_xnear,  view_cascade_ynear, -view_cascade_znear},  // top-left
                { view_cascade_xnear, -view_cascade_ynear, -view_cascade_znear},  // bottom-right
                {-view_cascade_xnear, -view_cascade_ynear, -view_cascade_znear},  // bottom-left

                // far face
                { view_cascade_xfar,   view_cascade_yfar,  -view_cascade_zfar},   // top-right
                {-view_cascade_xfar,   view_cascade_yfar,  -view_cascade_zfar},   // top-left
                { view_cascade_xfar,  -view_cascade_yfar,  -view_cascade_zfar},   // bottom-right
                {-view_cascade_xfar,  -view_cascade_yfar,  -view_cascade_zfar},   // bottom-left
            });

            // compute the bounds of the frustum in light space (the perspective of the light) by
            // projecting each frustum corner into light-space.
            const AABB light_bounds = bounding_aabb_of(view_frustum_corners, [&view_to_light](const Vector3& frustum_corner)
            {
                return transform_point(view_to_light, frustum_corner);
            });

            // because the light source is directional, the bounds of the corners in light space are
            // give-or-take equivalent to the bounds of the orthogonal projection cube corners.
            rv.push_back(OrthogonalProjectionParameters{
                .left   = light_bounds.min.x,
                .right  = light_bounds.max.x,
                .bottom = light_bounds.min.y,
                .top    = light_bounds.max.y,
                .near   = light_bounds.max.z,  // note: Z points opposite to the viewing direction
                .far    = light_bounds.min.z,
            });
        }
        return rv;
    }

    // returns a projection matrix for the given projection parameters
    Matrix4x4 to_matrix4x4(const OrthogonalProjectionParameters& p)
    {
        const float l = p.left;
        const float r = p.right;
        const float b = p.bottom;
        const float t = p.top;
        const float n = p.near;
        const float f = p.far;

        // Create a transform that maps the edges of the orthogonal proection to NDC (i.e. [-1.0, +1.0])
        return matrix4x4_cast(Transform{
            .scale       = {  2.0f/(r - l)  ,   2.0f/(t - b)  ,   2.0f/(f - n)  },
            .translation = {-(r + l)/(r - l), -(t + b)/(t - b), -(f + n)/(f - n)},
        });
    }
}

class osc::LOGLCSMTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/Guest/CSM"; }

    explicit Impl(LOGLCSMTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        // setup camera
        user_camera_.set_clipping_planes({0.1f, 10.0f});

        // ui
        log_viewer_.open();
    }

    void on_mount()
    {
        App::upd().make_main_loop_polling();
        user_camera_.on_mount();
    }

    void on_unmount()
    {
        user_camera_.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool on_event(Event& e)
    {
        return user_camera_.on_event(e);
    }

    void on_draw()
    {
        // update state from user inputs, window size, etc.
        user_camera_.on_draw();

        const auto cascade_projections = render_cascades(ui::get_main_window_workspace_aspect_ratio());
        render_scene_with_cascaded_shadow_mapping(cascade_projections);
        draw_debug_overlays();

        log_viewer_.on_draw();
    }

private:
    std::vector<Matrix4x4> render_cascades(float user_aspect_ratio)
    {
        // calculate how each cascade maps from the user's camera to light-space
        const auto cascade_projections = calculate_light_source_orthographic_projections(user_camera_, user_aspect_ratio, light_direction_);

        // for each of those mappings, render a cascade
        OSC_ASSERT_ALWAYS(cascade_projections.size() == cascade_rasters_.size());
        const Matrix4x4 world_to_light = look_at({0.0f, 0.0f, 0.0f}, Vector3{light_direction_}, {0.0f, 1.0f, 0.0f});

        std::vector<Matrix4x4> rv;
        rv.reserve(cascade_projections.size());
        for (size_t i = 0; i < cascade_projections.size(); ++i) {
            const Matrix4x4 cascade_projection_matrix = to_matrix4x4(cascade_projections[i]) * world_to_light;

            Camera camera;
            camera.set_view_matrix_override(identity<Matrix4x4>());
            camera.set_projection_matrix_override(cascade_projection_matrix);

            for (const auto& decoration : decorations_) {
                graphics::draw(decoration.mesh, decoration.transform, shadow_mapping_material_, camera);
            }

            camera.render_to(cascade_rasters_[i]);
            rv.push_back(cascade_projection_matrix);
        }
        return rv;
    }

    void render_scene_with_cascaded_shadow_mapping(std::span<const Matrix4x4> cascade_projections)
    {
        // setup material
        csm_material_.set_array("uLightWVP", cascade_projections);
        csm_material_.set("gNumPointLights", 0);
        csm_material_.set("gNumSpotLights", 0);
        csm_material_.set("gDirectionalLight.Base.Color", Color::white());
        csm_material_.set("gDirectionalLight.Base.AmbientIntensity", 0.5f);
        csm_material_.set("gDirectionalLight.Base.DiffuseIntensity", 0.9f);
        csm_material_.set("gDirectionalLight.Base.Direction", light_direction_);
        csm_material_.set("gDirectionalLight.Direction", light_direction_);
        csm_material_.set("gObjectColor", Color::dark_grey());
        csm_material_.set_array("gShadowMap", cascade_rasters_);
        csm_material_.set("gEyeWorldPos", user_camera_.position());
        csm_material_.set("gMatSpecularIntensity", 0.0f);
        csm_material_.set("gSpecularPower", 0.0f);

        // TODO: the clip space maths feels a bit wrong compared to just doing it in NDC?
        std::vector<float> ends;
        ends.reserve(c_normalized_cascade_planes.size()-1);
        for (size_t i = 1; i < c_normalized_cascade_planes.size(); ++i) {
            const auto [near, far] = user_camera_.clipping_planes();
            const Vector4 viewer_position = {0.0f, 0.0f, -lerp(near, far, c_normalized_cascade_planes[i]), 1.0f};
            const Matrix4x4 proj = user_camera_.projection_matrix(ui::get_main_window_workspace_aspect_ratio());
            const Vector4 proj_pos = (proj * viewer_position);
            ends.push_back(proj_pos.z);
        }
        csm_material_.set_array("gCascadeEndClipSpace", ends);

        for (const auto& decoration : decorations_) {
            graphics::draw(decoration.mesh, decoration.transform, csm_material_, user_camera_);
        }
        user_camera_.set_pixel_rect(ui::get_main_window_workspace_screen_space_rect());
        user_camera_.render_to_main_window();
    }

    void draw_debug_overlays()
    {
        const Vector2 overlay_dimensions{256.0f};

        Vector2 cursor = {0.0f, 0.0f};
        for ([[maybe_unused]] const auto& cascade_raster : cascade_rasters_) {
            // graphics::blit_to_screen(cascade_raster, Rect{cursor, cursor + overlay_dimensions});  // TODO
            cursor.x += overlay_dimensions.x;
        }
    }

    ResourceLoader resource_loader_ = App::resource_loader();
    MouseCapturingCamera user_camera_;
    std::vector<TransformedMesh> decorations_ = generate_decorations();
    MeshDepthWritingMaterial shadow_mapping_material_;
    Material csm_material_{Shader{
        resource_loader_.slurp("oscar_demos/learnopengl/shaders/Guest/CSM/lighting.vert"),
        resource_loader_.slurp("oscar_demos/learnopengl/shaders/Guest/CSM/lighting.frag"),
    }};
    Vector3 light_direction_ = normalize(Vector3{0.5f, -1.0f, 0.0f});
    std::vector<SharedDepthStencilRenderBuffer> cascade_rasters_ = generate_blank_cascade_buffers();

    // ui
    LogViewerPanel log_viewer_{&owner()};
};


osc::CStringView osc::LOGLCSMTab::id() { return Impl::static_label(); }
osc::LOGLCSMTab::LOGLCSMTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLCSMTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLCSMTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLCSMTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLCSMTab::impl_on_draw() { private_data().on_draw(); }
