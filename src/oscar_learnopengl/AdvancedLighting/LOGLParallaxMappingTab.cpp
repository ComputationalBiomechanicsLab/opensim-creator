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
    constexpr CStringView c_tab_string_id = "LearnOpenGL/ParallaxMapping";

    // matches the quad used in LearnOpenGL's parallax mapping tutorial
    Mesh generate_quad()
    {
        Mesh rv;
        rv.set_vertices({
            {-1.0f,  1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            { 1.0f, -1.0f, 0.0f},
            { 1.0f,  1.0f, 0.0f},
        });
        rv.set_normals({
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
        });
        rv.set_tex_coords({
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
        });
        rv.set_indices({
            0, 1, 2,
            0, 2, 3,
        });
        rv.recalculate_tangents();
        return rv;
    }

    MouseCapturingCamera create_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        return rv;
    }

    Material create_parallax_mapping_material(IResourceLoader& loader)
    {
        const Texture2D diffuse_map = load_texture2D_from_image(
            loader.open("oscar_learnopengl/textures/bricks2.jpg"),
            ColorSpace::sRGB
        );
        const Texture2D normal_map = load_texture2D_from_image(
            loader.open("oscar_learnopengl/textures/bricks2_normal.jpg"),
            ColorSpace::Linear
        );
        const Texture2D displacement_map = load_texture2D_from_image(
            loader.open("oscar_learnopengl/textures/bricks2_disp.jpg"),
            ColorSpace::Linear
        );

        Material rv{Shader{
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/ParallaxMapping.vert"),
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/ParallaxMapping.frag"),
        }};
        rv.set_texture("uDiffuseMap", diffuse_map);
        rv.set_texture("uNormalMap", normal_map);
        rv.set_texture("uDisplacementMap", displacement_map);
        rv.set<float>("uHeightScale", 0.1f);
        return rv;
    }

    Material create_lightcube_material(IResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_learnopengl/shaders/LightCube.vert"),
            loader.slurp("oscar_learnopengl/shaders/LightCube.frag"),
        }};
    }
}

class osc::LOGLParallaxMappingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_tab_string_id}
    {}

private:
    void impl_on_mount() final
    {
        camera_.on_mount();
    }

    void impl_on_unmount() final
    {
        camera_.on_unmount();
    }

    bool impl_on_event(const SDL_Event& e) final
    {
        return camera_.on_event(e);
    }

    void impl_on_draw() final
    {
        camera_.on_draw();

        // clear screen and ensure camera has correct pixel rect
        App::upd().clear_screen(Color::dark_grey());

        // draw normal-mapped quad
        {
            parallax_mapping_material_.set_vec3("uLightWorldPos", light_transform_.position);
            parallax_mapping_material_.set_vec3("uViewWorldPos", camera_.position());
            parallax_mapping_material_.set_bool("uEnableMapping", parallax_mapping_enabled_);
            graphics::draw(quad_mesh_, quad_transform_, parallax_mapping_material_, camera_);
        }

        // draw light source cube
        {
            light_cube_material_.set<Color>("uLightColor", Color::white());
            graphics::draw(cube_mesh_, light_transform_, light_cube_material_, camera_);
        }

        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());
        camera_.render_to_screen();

        ui::begin_panel("controls");
        ui::draw_checkbox("normal mapping", &parallax_mapping_enabled_);
        ui::end_panel();
    }

    ResourceLoader loader_ = App::resource_loader();

    // rendering state
    Material parallax_mapping_material_ = create_parallax_mapping_material(loader_);
    Material light_cube_material_ = create_lightcube_material(loader_);
    Mesh cube_mesh_ = BoxGeometry{};
    Mesh quad_mesh_ = generate_quad();

    // scene state
    MouseCapturingCamera camera_ = create_camera();
    Transform quad_transform_;
    Transform light_transform_ = {
        .scale = Vec3{0.2f},
        .position = {0.5f, 1.0f, 0.3f},
    };
    bool parallax_mapping_enabled_ = true;
};


CStringView osc::LOGLParallaxMappingTab::id()
{
    return c_tab_string_id;
}

osc::LOGLParallaxMappingTab::LOGLParallaxMappingTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLParallaxMappingTab::LOGLParallaxMappingTab(LOGLParallaxMappingTab&&) noexcept = default;
osc::LOGLParallaxMappingTab& osc::LOGLParallaxMappingTab::operator=(LOGLParallaxMappingTab&&) noexcept = default;
osc::LOGLParallaxMappingTab::~LOGLParallaxMappingTab() noexcept = default;

UID osc::LOGLParallaxMappingTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLParallaxMappingTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLParallaxMappingTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::LOGLParallaxMappingTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::LOGLParallaxMappingTab::impl_on_event(const SDL_Event& e)
{
    return impl_->on_event(e);
}

void osc::LOGLParallaxMappingTab::impl_on_draw()
{
    impl_->on_draw();
}
