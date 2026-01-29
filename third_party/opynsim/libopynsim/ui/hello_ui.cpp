#include "hello_ui.h"

#include <liboscar/oscar.h>

namespace ui = osc::ui;

namespace
{
    class HelloTriangleScreen final : public osc::Widget {
    public:
        HelloTriangleScreen()
        {
            // setup camera
            constexpr osc::Vector3 viewer_position = {3.0f, 0.0f, 0.0f};
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

        bool impl_on_event(osc::Event& e) override
        {
            return ui_context_.on_event(e);
        }

        void impl_on_draw() override
        {
            osc::App::upd().clear_main_window();

            ui_context_.on_start_new_frame();

            // ensure target texture matches screen dimensions
            target_texture_.reformat({
                .pixel_dimensions = osc::App::get().main_window_pixel_dimensions(),
                .device_pixel_ratio = osc::App::get().main_window_device_pixel_ratio(),
                .anti_aliasing_level = osc::App::get().anti_aliasing_level()
            });

            update_torus_if_params_changed();
            const auto seconds_since_startup = osc::App::get().frame_delta_since_startup().count();
            const osc::Transform transform = {
                .rotation = osc::angle_axis(osc::Radians{seconds_since_startup}, osc::CoordinateDirection::y())
            };
            osc::graphics::draw(mesh_, transform, material_, camera_);
            camera_.render_to(target_texture_);
            osc::graphics::blit_to_main_window(target_texture_);

            ui::begin_panel("window");
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
            mesh_ = osc::TorusKnotGeometry{edited_torus_parameters_};
            torus_parameters_ = edited_torus_parameters_;
        }

        ui::Context ui_context_{osc::App::upd()};
        osc::TorusKnotGeometryParams torus_parameters_;
        osc::TorusKnotGeometryParams edited_torus_parameters_;
        osc::TorusKnotGeometry mesh_;
        osc::Color torus_color_ = osc::Color::blue();
        osc::MeshPhongMaterial material_{{
            .ambient_color = 0.2f * torus_color_,
            .diffuse_color = 0.5f * torus_color_,
            .specular_color = 0.5f * torus_color_,
        }};
        osc::Camera camera_;
        osc::RenderTexture target_texture_;
    };
}

void opyn::show_hello_ui() { osc::App::main<HelloTriangleScreen>(osc::AppMetadata{}); }
