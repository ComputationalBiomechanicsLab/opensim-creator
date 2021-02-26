#include "application.hpp"

#include "src/config.hpp"
#include "src/opensim_bindings/fd_simulation.hpp"
#include "src/screens/loading_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/utils/os.hpp"

#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <OpenSim/Analyses/RegisterTypes_osimAnalyses.h>
#include <OpenSim/Common/RegisterTypes_osimCommon.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/RegisterTypes_osimSimulation.h>
#include <OpenSim/Tools/RegisterTypes_osimTools.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

using namespace osmv;

static const char usage[] = R"(usage: osmv [--help] [fd] MODEL.osim
)";
static const char help[] = R"(OPTIONS
    --help
        Show this help
)";

static bool skip_prefix(char const* prefix, char const* s, char const** out) {
    do {
        if (*prefix == '\0' and (*s == '\0' or *s == '=')) {
            *out = s;
            return true;
        }
    } while (*prefix++ == *s++);

    return false;
}

int main(int argc, char** argv) {
    --argc;
    ++argv;

    // handle named flag args (e.g. --help)
    while (argc) {
        char const* arg = *argv;

        if (*arg != '-') {
            break;
        }

        if (skip_prefix("--help", arg, &arg)) {
            std::cout << usage << '\n' << help << '\n';
            return EXIT_SUCCESS;
        }

        ++argv;
        --argc;
    }

    // pre-launch global inits
    {
        // install backtrace dumper
        //
        // useful if the application fails in prod: can provide some basic backtrace
        // info that users can paste into an issue or something, which is *a lot* more
        // information than "yeah, it's broke"
        osmv::install_backtrace_handler();

        // explicitly load OpenSim libs
        //
        // this is necessary because some compilers will refuse to link a library
        // unless symbols from that library are directly used.
        //
        // Unfortunately, OpenSim relies on weak linkage *and* static library-loading
        // side-effects. This means that (e.g.) the loading of muscles into the runtime
        // happens in a static initializer *in the library*.
        //
        // osmv may not link that library, though, because the source code in OSMV may
        // not *directly* use a symbol exported by the library (e.g. the code might use
        // OpenSim::Muscle references, but not actually concretely refer to a muscle
        // implementation method (e.g. a ctor)
        RegisterTypes_osimCommon();
        RegisterTypes_osimSimulation();
        RegisterTypes_osimActuators();
        RegisterTypes_osimAnalyses();
        RegisterTypes_osimTools();

        // globally set OpenSim's geometry search path
        //
        // when an osim file contains relative geometry path (e.g. "sphere.vtp"), the
        // OpenSim implementation will look in these directories for that file
        std::filesystem::path geometry_dir = osmv::config::resource_path("geometry");
        OpenSim::ModelVisualizer::addDirToGeometrySearchPaths(geometry_dir.string());
    }

    // init an application instance ready for rendering
    Application app;
    Application::set_current(&app);

    if (argc <= 0) {
        // no args: show splash screen
        app.start_render_loop<Splash_screen>();
    } else {
        // args: load args as osim files
        app.start_render_loop<Loading_screen>(argv[0]);
    }

    return EXIT_SUCCESS;
}
