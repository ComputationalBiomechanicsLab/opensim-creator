#include <oscar/oscar.h>

#include <emscripten.h>
#include <emscripten/html5.h>

using namespace osc;

namespace
{
    class HelloTriangleScreen final : public IScreen {
    public:
        HelloTriangleScreen()
        {
            const Vec3 viewer_position = {3.0f, 0.0f, 0.0f};
            camera_.set_position(viewer_position);
            camera_.set_direction({-1.0f, 0.0f, 0.0f});
            const Color color = Color::red();
            material_.set_ambient_color(0.2f * color);
            material_.set_diffuse_color(0.5f * color);
            material_.set_specular_color(0.5f * color);
            material_.set_viewer_position(viewer_position);
        }
    private:
         void impl_on_draw() override
         {
             const auto seconds_since_startup = App::get().frame_delta_since_startup().count();
             const auto transform = identity<Transform>().with_rotation(angle_axis(Radians{seconds_since_startup}, Vec3{0.0f, 1.0f, 0.0f}));
             graphics::draw(mesh_, transform, material_, camera_);
             camera_.render_to_screen();
         }

        TorusKnotGeometry mesh_;
        MeshPhongMaterial material_;
        Camera camera_;
    };

    EM_BOOL one_iter(double, void* userData) {
        static_cast<osc::App*>(userData)->tick();
        return EM_TRUE;
    }
}

int main(int, char**)
{
    osc::App* app = new osc::App{};
    app->set_screen<HelloTriangleScreen>();
    emscripten_request_animation_frame_loop(one_iter, app);
    return 0;
}
