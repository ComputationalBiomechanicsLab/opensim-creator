#include <oscar/oscar.h>

#ifdef EMSCRIPTEN
    #include <emscripten.h>
#endif

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

    struct TorusParameters final {
        float torus_radius = 1.0f;
        float tube_radius = 0.4f;

        friend bool operator==(const TorusParameters&, const TorusParameters&) = default;
    };

    class HelloTriangleScreen final : public IScreen {
    public:
        HelloTriangleScreen()
        {
            const Vec3 viewer_position = {3.0f, 0.0f, 0.0f};
            camera_.set_position(viewer_position);
            camera_.set_direction({-1.0f, 0.0f, 0.0f});
            const Color color = Color::blue();
            material_.set_ambient_color(0.2f * color);
            material_.set_diffuse_color(0.5f * color);
            material_.set_specular_color(0.5f * color);
            material_.set_viewer_position(viewer_position);
        }
    private:
        void impl_on_mount() override
        {
            // ui::context::init();
        }

        void impl_on_unmount() override
        {
            // ui::context::shutdown();
        }

        bool impl_on_event(const SDL_Event& e) override
        {
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    edited_torus_parameters_.torus_radius += 0.1f;
                    return true;
                case SDLK_RIGHT:
                    edited_torus_parameters_.torus_radius -= 0.1f;
                    return true;
                case SDLK_UP:
                    edited_torus_parameters_.tube_radius += 0.1f;
                    return true;
                case SDLK_DOWN:
                    edited_torus_parameters_.tube_radius -= 0.1f;
                    return true;
                }
            }

            return false; // return ui::context::on_event(e);
        }

        void impl_on_draw() override
        {
            // ui::context::on_start_new_frame();

            // ensure target texture matches screen dimensions
            {
                RenderTextureDescriptor descriptor{App::get().main_window_dimensions()};
                descriptor.set_anti_aliasing_level(App::get().anti_aliasing_level());
                target_texture_.reformat(descriptor);
            }
            update_torus_if_params_changed();
            const auto seconds_since_startup = App::get().frame_delta_since_startup().count();
            const auto transform = identity<Transform>().with_rotation(angle_axis(Radians{seconds_since_startup}, Vec3{0.0f, 1.0f, 0.0f}));
            graphics::draw(mesh_, transform, material_, camera_);
            camera_.render_to(target_texture_);
            graphics::blit_to_screen(target_texture_, Rect{{}, App::get().main_window_dimensions()}, gamma_correcter_);

            // ui::context::render();
        }

        void update_torus_if_params_changed()
        {
            if (torus_parameters_ == edited_torus_parameters_) {
                return;
            }
            mesh_ = TorusKnotGeometry{edited_torus_parameters_.torus_radius, edited_torus_parameters_.tube_radius};
            torus_parameters_ = edited_torus_parameters_;
        }

        TorusParameters torus_parameters_;
        TorusParameters edited_torus_parameters_;
        TorusKnotGeometry mesh_;
        MeshPhongMaterial material_;
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
    app.setup_main_loop<HelloTriangleScreen>();
    ScopeGuard guard{[&app](){ app.teardown_main_loop(); }};
    while (app.do_main_loop_step()) {
    }
    return 0;
#endif
}
