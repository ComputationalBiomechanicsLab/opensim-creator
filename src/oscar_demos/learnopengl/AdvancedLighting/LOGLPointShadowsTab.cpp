#include "LOGLPointShadowsTab.h"

#include <oscar/oscar.h>

#include <cmath>
#include <array>
#include <chrono>
#include <utility>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr Vec2i c_shadowmap_dimensions = {1024, 1024};

    Transform make_rotated_transform()
    {
        return {
            .scale = Vec3(0.75f),
            .rotation = angle_axis(60_deg, UnitVec3{1.0f, 0.0f, 1.0f}),
            .position = {-1.5f, 2.0f, -3.0f},
        };
    }

    struct SceneCube final {
        explicit SceneCube(Transform transform_) :
            transform{transform_}
        {}

        SceneCube(Transform transform_, bool invert_normals_) :
            transform{transform_},
            invert_normals{invert_normals_}
        {}

        Transform transform;
        bool invert_normals = false;
    };

    auto make_scene_cubes()
    {
        return std::to_array<SceneCube>({
            SceneCube{{.scale = Vec3{5.0f}}, true},
            SceneCube{{.scale = Vec3{0.50f}, .position = {4.0f, -3.5f, 0.0f}}},
            SceneCube{{.scale = Vec3{0.75f}, .position = {2.0f, 3.0f, 1.0f}}},
            SceneCube{{.scale = Vec3{0.50f}, .position = {-3.0f, -1.0f, 0.0f}}},
            SceneCube{{.scale = Vec3{0.50f}, .position = {-1.5f, 1.0f, 1.5f}}},
            SceneCube{make_rotated_transform()},
        });
    }

    RenderTexture create_depth_texture()
    {
        return RenderTexture{{
            .dimensions = c_shadowmap_dimensions,
            .dimensionality = TextureDimensionality::Cube,
            .color_format = ColorRenderBufferFormat::R32_SFLOAT,
        }};
    }

    MouseCapturingCamera create_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 5.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color(Color::clear());
        return rv;
    }
}

class osc::LOGLPointShadowsTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/AdvancedLighting/PointShadows"; }

    explicit Impl(LOGLPointShadowsTab& owner, Widget& parent) :
        TabPrivate{owner, &parent, static_label()}
    {}

    void on_mount()
    {
        App::upd().make_main_loop_polling();
        scene_camera_.on_mount();
    }

    void on_unmount()
    {
        scene_camera_.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool on_event(Event& e)
    {
        return scene_camera_.on_event(e);
    }

    void on_tick()
    {
        // move light position over time
        const double seconds = App::get().frame_delta_since_startup().count();
        light_pos_.x = static_cast<float>(3.0 * sin(0.5 * seconds));
    }

    void on_draw()
    {
        scene_camera_.on_draw();
        draw_3d_scene();
        draw_2d_ui();
    }

private:
    void draw_3d_scene()
    {
        const Rect viewport_screenspace_rect = ui::get_main_viewport_workspace_screenspace_rect();

        draw_shadow_pass_to_cubemap();
        draw_shadowmapped_scene_to_screen(viewport_screenspace_rect);
    }

    void draw_shadow_pass_to_cubemap()
    {
        // create a 90 degree cube cone projection matrix
        const float znear = 0.1f;
        const float zfar = 25.0f;
        const Mat4 projection_matrix = perspective(
            90_deg,
            aspect_ratio_of(c_shadowmap_dimensions),
            znear,
            zfar
        );

        // have the cone point toward all 6 faces of the cube
        const auto shadow_matrices =
            calc_cubemap_view_proj_matrices(projection_matrix, light_pos_);

        // pass data to material
        shadowmapping_material_.set_array("uShadowMatrices", shadow_matrices);
        shadowmapping_material_.set("uLightPos", light_pos_);
        shadowmapping_material_.set("uFarPlane", zfar);

        // render (shadowmapping does not use the camera's view/projection matrices)
        Camera camera;
        for (const SceneCube& cube : scene_cubes_) {
            graphics::draw(cube_mesh_, cube.transform, shadowmapping_material_, camera);
        }
        camera.render_to(depth_texture_);
    }

    void draw_shadowmapped_scene_to_screen(const Rect& viewport_screenspace_rect)
    {
        Material material = use_soft_shadows_ ? soft_scene_material_ : scene_material_;

        // set shared material params
        material.set("uDiffuseTexture", m_WoodTexture);
        material.set("uLightPos", light_pos_);
        material.set("uViewPos", scene_camera_.position());
        material.set("uFarPlane", 25.0f);
        material.set("uShadows", soft_shadows_);

        material.set("uDepthMap", depth_texture_);
        for (const SceneCube& cube : scene_cubes_) {
            MaterialPropertyBlock material_props;
            material_props.set("uReverseNormals", cube.invert_normals);  // UNDOME
            graphics::draw(cube_mesh_, cube.transform, material, scene_camera_, std::move(material_props));
        }
        material.unset("uDepthMap");

        // also, draw the light as a little cube
        material.set("uShadows", soft_shadows_);
        graphics::draw(cube_mesh_, {.scale = Vec3{0.1f}, .position = light_pos_}, material, scene_camera_);

        scene_camera_.set_pixel_rect(viewport_screenspace_rect);
        scene_camera_.render_to_screen();
        scene_camera_.set_pixel_rect(std::nullopt);
    }

    void draw_2d_ui()
    {
        ui::begin_panel("controls");
        ui::draw_checkbox("show shadows", &soft_shadows_);
        ui::draw_checkbox("soften shadows", &use_soft_shadows_);
        ui::end_panel();

        perf_panel_.on_draw();
    }

    ResourceLoader loader_ = App::resource_loader();

    Material shadowmapping_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/point_shadows/MakeShadowMap.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/point_shadows/MakeShadowMap.geom"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/point_shadows/MakeShadowMap.frag"),
    }};

    Material scene_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/point_shadows/Scene.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/point_shadows/Scene.frag"),
    }};

    Material soft_scene_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/point_shadows/Scene.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/point_shadows/SoftScene.frag"),
    }};

    MouseCapturingCamera scene_camera_ = create_camera();
    Texture2D m_WoodTexture = load_texture2D_from_image(
        loader_.open("oscar_demos/learnopengl/textures/wood.jpg"),
        ColorSpace::sRGB
    );
    Mesh cube_mesh_ = BoxGeometry{{.width = 2.0f, .height = 2.0f, .depth = 2.0f}};
    std::array<SceneCube, 6> scene_cubes_ = make_scene_cubes();
    RenderTexture depth_texture_ = create_depth_texture();
    Vec3 light_pos_ = {};
    bool soft_shadows_ = true;
    bool use_soft_shadows_ = false;

    PerfPanel perf_panel_{"Perf"};
};


CStringView osc::LOGLPointShadowsTab::id() { return Impl::static_label(); }

osc::LOGLPointShadowsTab::LOGLPointShadowsTab(Widget& parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLPointShadowsTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLPointShadowsTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLPointShadowsTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLPointShadowsTab::impl_on_tick() { private_data().on_tick(); }
void osc::LOGLPointShadowsTab::impl_on_draw() { private_data().on_draw(); }
