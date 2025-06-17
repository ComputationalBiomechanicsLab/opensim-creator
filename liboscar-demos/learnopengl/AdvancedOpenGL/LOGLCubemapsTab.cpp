#include "LOGLCubemapsTab.h"

#include <liboscar/oscar.h>

#include <array>
#include <memory>
#include <optional>
#include <string_view>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_skybox_texture_filenames = std::to_array<std::string_view>({
        "skybox_right.jpg",
        "skybox_left.jpg",
        "skybox_top.jpg",
        "skybox_bottom.jpg",
        "skybox_front.jpg",
        "skybox_back.jpg",
    });
    static_assert(c_skybox_texture_filenames.size() == num_options<CubemapFace>());

    Cubemap load_cubemap(ResourceLoader& loader)
    {
        // load the first face, so we know the width
        Texture2D face_texture = load_texture2D_from_image(
            loader.open(ResourcePath{"oscar_demos/learnopengl/textures"} / c_skybox_texture_filenames.front()),
            ColorSpace::sRGB
        );

        const Vec2i texture_dimensions = face_texture.pixel_dimensions();
        OSC_ASSERT(texture_dimensions.x == texture_dimensions.y);

        // load all face data into the cubemap
        static_assert(num_options<CubemapFace>() == c_skybox_texture_filenames.size());

        const auto cubemap_faces = make_option_iterable<CubemapFace>();
        auto face_iterator = cubemap_faces.begin();
        Cubemap cubemap{texture_dimensions.x, face_texture.texture_format()};
        cubemap.set_pixel_data(*face_iterator++, face_texture.pixel_data());
        for (; face_iterator != cubemap_faces.end(); ++face_iterator)
        {
            face_texture = load_texture2D_from_image(
                loader.open(ResourcePath{"oscar_demos/learnopengl/textures"} / c_skybox_texture_filenames[to_index(*face_iterator)]),
                ColorSpace::sRGB
            );
            OSC_ASSERT(face_texture.pixel_dimensions().x == texture_dimensions.x);
            OSC_ASSERT(face_texture.pixel_dimensions().y == texture_dimensions.x);
            OSC_ASSERT(face_texture.texture_format() == cubemap.texture_format());
            cubemap.set_pixel_data(*face_iterator, face_texture.pixel_data());
        }

        return cubemap;
    }

    MouseCapturingCamera create_camera_that_matches_learnopengl()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_field_of_view(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    struct CubeMaterial final {
        CStringView label;
        Material material;
    };

    std::array<CubeMaterial, 3> create_cube_materials(ResourceLoader& loader)
    {
        return std::to_array({
            CubeMaterial{
                "Basic",
                Material{Shader{
                    loader.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Cubemaps/Basic.vert"),
                    loader.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Cubemaps/Basic.frag"),
                }},
            },
            CubeMaterial{
                "Reflection",
                Material{Shader{
                    loader.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Cubemaps/Reflection.vert"),
                    loader.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Cubemaps/Reflection.frag"),
                }},
            },
            CubeMaterial{
                "Refraction",
                Material{Shader{
                    loader.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Cubemaps/Refraction.vert"),
                    loader.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Cubemaps/Refraction.frag"),
                }},
            },
        });
    }
}

class osc::LOGLCubemapsTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/AdvancedOpenGL/Cubemaps"; }

    explicit Impl(LOGLCubemapsTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        for (CubeMaterial& cube_material : cube_materials_) {
            cube_material.material.set("uTexture", container_texture_);
            cube_material.material.set("uSkybox", cubemap_);
        }

        // set the depth function to LessOrEqual because the skybox shader
        // performs a trick in which it sets gl_Position = v.xyww in order
        // to guarantee that the depth of all fragments in the skybox is
        // the highest possible depth, so that it fails an early depth
        // test if anything is drawn over it in the scene (reduces
        // fragment shader pressure)
        skybox_material_.set("uSkybox", cubemap_);
        skybox_material_.set_depth_function(DepthFunction::LessOrEqual);
    }

    void on_mount()
    {
        App::upd().make_main_loop_polling();
        camera_.on_mount();
    }

    void on_unmount()
    {
        camera_.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool on_event(Event& e)
    {
        return camera_.on_event(e);
    }

    void on_draw()
    {
        camera_.on_draw();

        // clear screen and ensure camera has correct pixel rect
        camera_.set_pixel_rect(ui::get_main_window_workspace_screen_space_rect());

        draw_scene_cube();
        draw_skybox();
        draw_2d_ui();
    }

private:
    void draw_scene_cube()
    {
        cube_properties_.set("uCameraPos", camera_.position());
        cube_properties_.set("uIOR", ior_);
        graphics::draw(
            cube_mesh_,
            identity<Transform>(),
            cube_materials_.at(cube_material_index_).material,
            camera_,
            cube_properties_
        );
        camera_.render_to_main_window();
    }

    void draw_skybox()
    {
        camera_.set_clear_flags(CameraClearFlag::None);
        camera_.set_view_matrix_override(Mat4{Mat3{camera_.view_matrix()}});
        graphics::draw(
            skybox_,
            identity<Transform>(),
            skybox_material_,
            camera_
        );
        camera_.render_to_main_window();
        camera_.set_view_matrix_override(std::nullopt);
        camera_.set_clear_flags(CameraClearFlag::Default);
    }

    void draw_2d_ui()
    {
        ui::begin_panel("controls");
        if (ui::begin_combobox("Cube Texturing", cube_materials_.at(cube_material_index_).label)) {
            for (size_t i = 0; i < cube_materials_.size(); ++i) {
                bool selected = i == cube_material_index_;
                if (ui::draw_selectable(cube_materials_[i].label, &selected)) {
                    cube_material_index_ = i;
                }
            }
            ui::end_combobox();
        }
        ui::draw_float_input("IOR", &ior_);
        ui::end_panel();
    }

    ResourceLoader loader_ = App::resource_loader();

    std::array<CubeMaterial, 3> cube_materials_ = create_cube_materials(loader_);
    size_t cube_material_index_ = 0;
    MaterialPropertyBlock cube_properties_;
    Mesh cube_mesh_ = BoxGeometry{};
    Texture2D container_texture_ = load_texture2D_from_image(
        loader_.open("oscar_demos/learnopengl/textures/container.jpg"),
        ColorSpace::sRGB
    );
    float ior_ = 1.52f;

    Material skybox_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Cubemaps/Skybox.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Cubemaps/Skybox.frag"),
    }};
    Mesh skybox_ = BoxGeometry{{.width = 2.0f, .height = 2.0f, .depth = 2.0f}};
    Cubemap cubemap_ = load_cubemap(loader_);

    MouseCapturingCamera camera_ = create_camera_that_matches_learnopengl();
};


CStringView osc::LOGLCubemapsTab::id() { return Impl::static_label(); }

osc::LOGLCubemapsTab::LOGLCubemapsTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLCubemapsTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLCubemapsTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLCubemapsTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLCubemapsTab::impl_on_draw() { private_data().on_draw(); }
