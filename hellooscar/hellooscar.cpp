#include <liboscar/oscar.h>

#ifdef EMSCRIPTEN
    #include <emscripten.h>
#endif
#include <liboscar/Utils/Assertions.h>

#include <array>
#include <cstdlib>
#include <new>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace osc;

namespace
{
    constexpr CStringView c_gamma_correcting_vertex_shader_src = R"(
        #version 330 core

        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;

        out vec2 TexCoord;

        void main()
        {
            TexCoord = aTexCoord;
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    constexpr CStringView c_gamma_correcting_fragment_shader_src = R"(
        #version 330 core

        uniform sampler2D uTexture;

        in vec2 TexCoord;
        out vec4 FragColor;

        void main()
        {
            // linear --> sRGB
            vec4 linearColor = texture(uTexture, TexCoord);
            FragColor = vec4(pow(linearColor.rgb, vec3(1.0/2.2)), linearColor.a);
        }
    )";

    class HelloTriangleScreen final : public Screen {
    public:
        HelloTriangleScreen()
        {
            // setup camera
            const Vec3 viewer_position = {3.0f, 0.0f, 0.0f};
            camera_.set_position(viewer_position);
            camera_.set_direction({-1.0f, 0.0f, 0.0f});

            // setup material
            material_.set_viewer_position(viewer_position);
        }
    private:
        void impl_on_mount() override
        {
            ui::context::init(App::upd());
        }

        void impl_on_unmount() override
        {
            ui::context::shutdown(App::upd());
        }

        bool impl_on_event(Event& e) override
        {
            return ui::context::on_event(e);
        }

        void impl_on_draw() override
        {
            App::upd().clear_screen();

            ui::context::on_start_new_frame(App::upd());

            // ensure target texture matches screen dimensions
            target_texture_.reformat({
                .dimensions = App::get().main_window_dimensions(),
                .anti_aliasing_level = App::get().anti_aliasing_level()
            });

            update_torus_if_params_changed();
            const auto seconds_since_startup = App::get().frame_delta_since_startup().count();
            const auto transform = identity<Transform>().with_rotation(angle_axis(Radians{seconds_since_startup}, Vec3{0.0f, 1.0f, 0.0f}));
            graphics::draw(mesh_, transform, material_, camera_);
            camera_.render_to(target_texture_);

            if (App::get().is_main_window_gamma_corrected()) {
                graphics::blit_to_screen(target_texture_, Rect{{}, App::get().main_window_dimensions()});
            }
            else {
                graphics::blit_to_screen(target_texture_, Rect{{}, App::get().main_window_dimensions()}, gamma_correcter_);
            }

            ui::begin_panel("window");
            ui::draw_text("source code");
            ui::draw_float_slider("torus_radius", &edited_torus_parameters_.torus_radius, 0.0f, 5.0f);
            ui::draw_float_slider("tube_radius", &edited_torus_parameters_.tube_radius, 0.0f, 5.0f);
            ui::draw_size_t_input("p", &edited_torus_parameters_.p);
            ui::draw_size_t_input("q", &edited_torus_parameters_.q);
            ui::end_panel();

            ui::context::render();
        }

        void update_torus_if_params_changed()
        {
            if (torus_parameters_ == edited_torus_parameters_) {
                return;
            }
            mesh_ = TorusKnotGeometry{edited_torus_parameters_};
            torus_parameters_ = edited_torus_parameters_;
        }

        TorusKnotGeometryParams torus_parameters_;
        TorusKnotGeometryParams edited_torus_parameters_;
        TorusKnotGeometry mesh_;
        Color torus_color_ = Color::blue();
        MeshPhongMaterial material_{{
            .ambient_color = 0.2f * torus_color_,
            .diffuse_color = 0.5f * torus_color_,
            .specular_color = 0.5f * torus_color_,
        }};
        Material gamma_correcter_{Shader{c_gamma_correcting_vertex_shader_src, c_gamma_correcting_fragment_shader_src}};
        Camera camera_;
        RenderTexture target_texture_;
    };
}


int main(int, char**)
{
#ifdef EMSCRIPTEN
    osc::App* app = new osc::App{};
    app->setup_main_loop<HelloTriangleScreen>();
    emscripten_set_main_loop_arg([](void* ptr) -> void
    {
        if (not static_cast<App*>(ptr)->do_main_loop_step()) {
            throw std::runtime_error{"exit"};
        }
    }, app, 0, EM_TRUE);
    return 0;
#else
    osc::App app;
    app.show<HelloTriangleScreen>();
    return 0;
#endif
}
