#include <liboscar/oscar.h>

using namespace osc;

namespace
{
    class HelloTriangleScreen final : public Widget {
    public:
        HelloTriangleScreen()
        {
            // setup camera
            constexpr Vec3 viewer_position = {3.0f, 0.0f, 0.0f};
            camera_.set_position(viewer_position);
            camera_.set_direction({-1.0f, 0.0f, 0.0f});

            // setup material
            material_.set_viewer_position(viewer_position);
        }
    private:
        void impl_on_mount() override
        {}

        void impl_on_unmount() override
        {}

        bool impl_on_event(Event& e) override
        {
            return ui_context_.on_event(e);
        }

        void impl_on_draw() override
        {
            App::upd().clear_main_window();

            ui_context_.on_start_new_frame();

            // ensure target texture matches screen dimensions
            target_texture_.reformat({
                .dimensions = App::get().main_window_pixel_dimensions(),
                .device_pixel_ratio = App::get().main_window_device_pixel_ratio(),
                .anti_aliasing_level = App::get().anti_aliasing_level()
            });

            update_torus_if_params_changed();
            const auto seconds_since_startup = App::get().frame_delta_since_startup().count();
            const auto transform = identity<Transform>().with_rotation(angle_axis(Radians{seconds_since_startup}, Vec3{0.0f, 1.0f, 0.0f}));
            graphics::draw(mesh_, transform, material_, camera_);
            camera_.render_to(target_texture_);
            graphics::blit_to_main_window(target_texture_);

            ui::begin_panel("window");
            ui::draw_text("source code");
            ui::draw_float_slider("torus_radius", &edited_torus_parameters_.torus_radius, 0.0f, 5.0f);
            ui::draw_float_slider("tube_radius", &edited_torus_parameters_.tube_radius, 0.0f, 5.0f);
            ui::draw_size_t_input("p", &edited_torus_parameters_.p);
            ui::draw_size_t_input("q", &edited_torus_parameters_.q);
            ui::end_panel();

            ui_context_.render();
        }

        void update_torus_if_params_changed()
        {
            if (torus_parameters_ == edited_torus_parameters_) {
                return;
            }
            mesh_ = TorusKnotGeometry{edited_torus_parameters_};
            torus_parameters_ = edited_torus_parameters_;
        }

        ui::Context ui_context_{App::upd()};
        TorusKnotGeometryParams torus_parameters_;
        TorusKnotGeometryParams edited_torus_parameters_;
        TorusKnotGeometry mesh_;
        Color torus_color_ = Color::blue();
        MeshPhongMaterial material_{{
            .ambient_color = 0.2f * torus_color_,
            .diffuse_color = 0.5f * torus_color_,
            .specular_color = 0.5f * torus_color_,
        }};
        Camera camera_;
        RenderTexture target_texture_;
    };
}

int main(int, char**)
{
    return App::main<HelloTriangleScreen>(AppMetadata{});
}
