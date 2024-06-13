#include <oscar/oscar.h>

using namespace osc;

namespace
{
    class HelloTriangleScreen final : public IScreen {
    public:
         void impl_on_draw() override
         {
            Camera camera;
            graphics::draw(mesh_, identity<Transform>(), material_, camera);
            camera.render_to_screen();
         }
    private:
        Mesh mesh_ = TorusKnotGeometry{};
        Material material_ = MeshBasicMaterial{};
    };
}

int main(int, char**)
{
    osc::App app;
    app.show<HelloTriangleScreen>();
    return 0;
}
