#include "src/app.hpp"
#include "src/log.hpp"
#include "src/screens/tut4_hittesting_screen.hpp"
#include "src/screens/tut5_mesh_hittesting.hpp"

using namespace osc;

int main(int, char**) {

    try {
        App app;
        //app.show<Tut4_hittesting_screen>();
        app.show<Tut5_mesh_hittesting>();
    } catch (std::exception const& ex) {
        log::info("exception thrown to root of application: %s", ex.what());
        return -1;
    }

    return 0;
}
