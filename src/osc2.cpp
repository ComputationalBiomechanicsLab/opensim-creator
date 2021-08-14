#include "src/app.hpp"
#include "src/log.hpp"

#include "src/screens/experimental/simbody_meshgen_screen.hpp"

using namespace osc;

int main(int, char**) {
    try {
        App app;
        app.show<Simbody_meshgen_screen>();
    } catch (std::exception const& ex) {
        log::info("exception thrown to root of application: %s", ex.what());
        return -1;
    }

    return 0;
}
