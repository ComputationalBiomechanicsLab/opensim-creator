#include "LOGLCSMTab.h"

#include <oscar/oscar.h>

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
    constexpr CStringView c_tab_string_id = "LearnOpenGL/CSM";

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
        auto dist = std::normal_distribution{0.1f, 0.2f};
        const AABB grid_bounds = {{-5.0f,  0.0f, -5.0f}, {5.0f, 0.0f, 5.0f}};
        const Vec3 grid_dimensions = dimensions_of(grid_bounds);
        const Vec2uz num_grid_cells = {10, 10};

        std::vector<TransformedMesh> rv;
        rv.reserve(num_grid_cells.x * num_grid_cells.y);

        for (size_t x = 0; x < num_grid_cells.x; ++x) {
            for (size_t y = 0; y < num_grid_cells.y; ++y) {

                const Vec3 cell_pos = grid_bounds.min + grid_dimensions * (Vec3{x, 0.0f, y} / Vec3{num_grid_cells.x - 1_uz, 1, num_grid_cells.y - 1_uz});
                Mesh mesh;
                rgs::sample(possible_geometries, &mesh, 1, rng);

                rv.push_back(TransformedMesh{
                    .mesh = mesh,
                    .transform = {
                        .scale = Vec3{abs(dist(rng))},
                        .position = cell_pos,
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
                .position = {0.0f, -1.0f, 0.0f},
            },
        });

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
    std::vector<OrthogonalProjectionParameters> calculate_light_source_orthographic_projections(
        const Camera& camera,
        float aspect_ratio,
        UnitVec3 light_direction)
    {
        // most of the maths/logic here was ported from an excellently-written ogldev tutorial:
        //
        // - https://ogldev.org/www/tutorial49/tutorial49.html

        // normalized means that 0.0 == near and 1.0 == far
        //
        // these planes are paired to figure out the near/far planes of each CSM's frustum
        constexpr auto normalized_cascade_planes = std::to_array({ 0.0f, 1.0f/3.0f, 2.0f/3.0f, 3.0f/3.0f });

        // precompute transforms
        const Mat4 model2light = look_at({0.0f, 0.0f, 0.0f}, Vec3{light_direction}, {0.0f, 1.0f, 0.0f});
        const Mat4 view2model = inverse(camera.view_matrix());
        const Mat4 view2light = model2light * view2model;

        // precompute necessary values to figure out the corners of the view frustum
        const auto [view_znear, view_zfar] = camera.clipping_planes();
        const Radians view_vfov = camera.vertical_fov();
        const Radians view_hfov = vertial_to_horizontal_fov(view_vfov, aspect_ratio);
        const float view_tan_half_vfov = tan(0.5f * view_vfov);
        const float view_tan_half_hfov = tan(0.5f * view_hfov);

        // calculate `OrthogonalProjectionParameters` for each cascade
        std::vector<OrthogonalProjectionParameters> rv;
        rv.reserve(normalized_cascade_planes.size() - 1);
        for (size_t i = 0; i < normalized_cascade_planes.size()-1; ++i) {
            const float view_cascade_znear = lerp(view_znear, view_zfar, normalized_cascade_planes[i]);
            const float view_cascade_zfar = lerp(view_znear, view_zfar, normalized_cascade_planes[i+1]);

            // imagine a triangle with a point where the viewer is (0,0,0 in view-space) and another
            // point thats (e.g.) znear away from the viewer: the FOV dictates the angle of the corner
            // that originates from the viewer
            const float view_cascade_xnear = view_cascade_znear * view_tan_half_hfov;
            const float view_cascade_xfar  = view_cascade_zfar  * view_tan_half_hfov;
            const float view_cascade_ynear = view_cascade_znear * view_tan_half_vfov;
            const float view_cascade_yfar  = view_cascade_zfar  * view_tan_half_vfov;

            const FrustumCorners view_frustum_corners = {
                // near face
                Vec3{ view_cascade_xnear,  view_cascade_ynear, view_cascade_znear},  // top-right
                Vec3{-view_cascade_xnear,  view_cascade_ynear, view_cascade_znear},  // top-left
                Vec3{ view_cascade_xnear, -view_cascade_ynear, view_cascade_znear},  // bottom-right
                Vec3{-view_cascade_xnear, -view_cascade_ynear, view_cascade_znear},  // bottom-left

                // far face
                Vec3{ view_cascade_xfar,  view_cascade_yfar, view_cascade_zfar},     // top-right
                Vec3{-view_cascade_xfar,  view_cascade_yfar, view_cascade_zfar},     // top-left
                Vec3{ view_cascade_xfar, -view_cascade_yfar, view_cascade_zfar},     // bottom-right
                Vec3{-view_cascade_xfar, -view_cascade_yfar, view_cascade_zfar},     // bottom-left
            };

            // compute the bounds in light-space by projecting each corner into light-space and min-maxing
            Vec3 light_bounds_min = transform_point(view2light, view_frustum_corners.front());
            Vec3 light_bounds_max = light_bounds_min;
            for (size_t corner = 1; corner < view_frustum_corners.size(); ++corner) {
                const Vec3 lightCorner = transform_point(view2light, view_frustum_corners[corner]);
                light_bounds_min = elementwise_min(light_bounds_min, lightCorner);
                light_bounds_max = elementwise_max(light_bounds_max, lightCorner);
            }

            // then use those bounds to compute the orthogonal projection parameters of
            // the directional light
            rv.push_back(OrthogonalProjectionParameters{
                .r = light_bounds_max.x,
                .l = light_bounds_min.x,
                .b = light_bounds_min.y,
                .t = light_bounds_max.y,
                .f = light_bounds_max.z,
                .n = light_bounds_min.z,
            });
        }
        return rv;
    }

    // returns a projection matrix for the given projection parameters
    Mat4 to_mat4(const OrthogonalProjectionParameters& p)
    {
        // from: https://github.com/emeiri/ogldev/blob/master/Common/math_3d.cpp#L290
        //
        // note: ogldev uses row-major matrices

        Mat4 m;

        float l = p.l;
        float r = p.r;
        float b = p.b;
        float t = p.t;
        float n = p.n;
        float f = p.f;

        m[0][0] = 2.0f/(r - l); m[0][1] = 0.0f;         m[0][2] = 0.0f;         m[0][3] = -(r + l)/(r - l);
        m[1][0] = 0.0f;         m[1][1] = 2.0f/(t - b); m[1][2] = 0.0f;         m[1][3] = -(t + b)/(t - b);
        m[2][0] = 0.0f;         m[2][1] = 0.0f;         m[2][2] = 2.0f/(f - n); m[2][3] = -(f + n)/(f - n);
        m[3][0] = 0.0f;         m[3][1] = 0.0f;         m[3][2] = 0.0f;         m[3][3] = 1.0;

        m = transpose(m);  // the above is row-major

        return m;
    }
}

class osc::LOGLCSMTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_tab_string_id}
    {
        // setup camera
        user_camera_.set_clipping_planes({0.1f, 10.0f});

        // ui
        log_viewer_.open();
    }

private:
    void impl_on_mount() final
    {
        App::upd().make_main_loop_polling();
        user_camera_.on_mount();
    }

    void impl_on_unmount() final
    {
        user_camera_.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool impl_on_event(const Event& e) final
    {
        return user_camera_.on_event(e);
    }

    void impl_on_draw() final
    {
        // update state from user inputs, window size, etc.
        user_camera_.on_draw();

        const auto cascade_projections = render_cascades(ui::get_main_viewport_workspace_aspect_ratio());
        render_scene_with_cascaded_shadow_mapping(cascade_projections);
        draw_debug_overlays();

        log_viewer_.on_draw();
    }

    std::vector<Mat4> render_cascades(float user_aspect_ratio)
    {
        // calculate how each cascade maps from the user's camera to light-space
        const auto cascade_projections = calculate_light_source_orthographic_projections(user_camera_, user_aspect_ratio, light_direction_);

        // for each of those mappings, render a cascade
        OSC_ASSERT_ALWAYS(cascade_projections.size() == cascade_rasters_.size());
        std::vector<Mat4> rv;
        rv.reserve(cascade_projections.size());
        for (size_t i = 0; i < cascade_projections.size(); ++i) {
            const auto& cascade_projection = cascade_projections[i];
            const Mat4 cascade_projection_mat4 = to_mat4(cascade_projection);

            Camera light_camera;
            light_camera.set_position({});
            light_camera.set_direction(light_direction_);
            light_camera.set_projection_matrix_override(cascade_projection_mat4);

            shadowmapping_material_.set_color(Color::clear().with_element(i, 1.0f));
            for (const auto& decoration : decorations_) {
                graphics::draw(decoration.mesh, decoration.transform, shadowmapping_material_, light_camera);
            }

            light_camera.render_to(cascade_rasters_[i]);
            rv.push_back(cascade_projection_mat4);
        }
        return rv;
    }

    void render_scene_with_cascaded_shadow_mapping(std::span<const Mat4> cascade_projections)
    {
        // setup material
        csm_material_.set_array<Mat4>("uLightWVP", cascade_projections);
        csm_material_.set("gNumPointLights", 0);
        csm_material_.set("gNumSpotLights", 0);
        csm_material_.set("gDirectionalLight.Base.Color", Color::white());
        csm_material_.set("gDirectionalLight.Base.AmbientIntensity", 0.5f);
        csm_material_.set("gDirectionalLight.Base.DiffuseIntensity", 0.9f);
        csm_material_.set("gDirectionalLight.Base.Direction", Vec3{1.0f, -1.0f, 0.0f});
        csm_material_.set("gObjectColor", Color::orange());
        // csm_material_.set_array<RenderTexture>("gShadowMap", cascade_rasters_);  // TODO
        csm_material_.set("gEyeWorldPos", user_camera_.position());
        csm_material_.set("gMatSpecularIntensity", 0.0f);
        csm_material_.set("gSpecularPower", 0.0f);
        csm_material_.set_array<float>("gCascadeEndClipSpace", {{-0.333f, +0.333f, 1.0f}});  // TODO

        for (const auto& decoration : decorations_) {
            graphics::draw(decoration.mesh, decoration.transform, csm_material_, user_camera_);
        }
        user_camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());
        user_camera_.render_to_screen();
    }

    void draw_debug_overlays()
    {
        const Vec2 overlay_dimensions{256.0f};

        Vec2 cursor = {0.0f, 0.0f};
        for (const auto& cascade_raster : cascade_rasters_) {
            graphics::blit_to_screen(cascade_raster, Rect{cursor, cursor + overlay_dimensions});
            cursor.x += overlay_dimensions.x;
        }
    }

    ResourceLoader resource_loader_ = App::resource_loader();
    MouseCapturingCamera user_camera_;
    std::vector<TransformedMesh> decorations_ = generate_decorations();
    MeshBasicMaterial shadowmapping_material_{{
        .color = Color::red(),  // TODO: should be depth-only
    }};
    Material csm_material_{Shader{
        resource_loader_.slurp("oscar_learnopengl/shaders/Guest/CSM/lighting.vert"),
        resource_loader_.slurp("oscar_learnopengl/shaders/Guest/CSM/lighting.frag"),
    }};
    UnitVec3 light_direction_{0.5f, -1.0f, 0.0f};
    std::vector<RenderTexture> cascade_rasters_ = std::vector<RenderTexture>(3, RenderTexture{{.dimensions = {256, 256}}});

    // ui
    LogViewerPanel log_viewer_{"log"};
};


osc::CStringView osc::LOGLCSMTab::id()
{
    return c_tab_string_id;
}

osc::LOGLCSMTab::LOGLCSMTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLCSMTab::LOGLCSMTab(LOGLCSMTab&&) noexcept = default;
osc::LOGLCSMTab& osc::LOGLCSMTab::operator=(LOGLCSMTab&&) noexcept = default;
osc::LOGLCSMTab::~LOGLCSMTab() noexcept = default;

UID osc::LOGLCSMTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLCSMTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLCSMTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::LOGLCSMTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::LOGLCSMTab::impl_on_event(const Event& e)
{
    return impl_->on_event(e);
}

void osc::LOGLCSMTab::impl_on_draw()
{
    impl_->on_draw();
}
