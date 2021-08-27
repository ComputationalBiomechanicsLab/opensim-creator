#include "src/screens/cookiecutter_screen.hpp"
#include "src/screens/model_editor_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/screens/experimental/hellotriangle_screen.hpp"
#include "src/screens/experimental/hittest_screen.hpp"
#include "src/screens/experimental/mesh_hittest_with_bvh_screen.hpp"
#include "src/screens/experimental/instanced_renderer_screen.hpp"
#include "src/screens/experimental/mesh_hittest_screen.hpp"
#include "src/screens/experimental/meshes_to_model_wizard_screen.hpp"
#include "src/screens/experimental/simbody_meshgen_screen.hpp"
#include "src/screens/experimental/imguizmo_demo_screen.hpp"
#include "src/screens/experimental/experiments_screen.hpp"
#include "src/screens/experimental/opensim_modelstate_decoration_generator_screen.hpp"
#include "src/screens/experimental/component_3d_viewer_screen.hpp"
#include "src/app.hpp"
#include "src/log.hpp"
#include "src/main_editor_state.hpp"

using namespace osc;

int main(int, char**) {
    try {
        App app;
        //app.show<Hellotriangle_screen>();
        //app.show<Hittest_screen>();
        //app.show<Mesh_hittest_with_bvh_screen>();
        //app.show<Instanced_render_screen>();
        //app.show<Mesh_hittest_with_bvh_screen>();
        //app.show<Simbody_meshgen_screen>();
        //app.show<Imguizmo_demo_screen>();
        //app.show<cookiecutter_screen>();
        //app.show<Opensim_modelstate_decoration_generator_screen>();
        //app.show<Experiments_screen>();
        //app.show<Component_3d_viewer_screen>();
        //app.show<Splash_screen>();
        //app.show<Meshes_to_model_wizard_screen>();
        app.show<Model_editor_screen>(std::make_shared<Main_editor_state>());
    } catch (std::exception const& ex) {
        log::info("exception thrown to root of application: %s", ex.what());
        return -1;
    }

    return 0;
}
