#include "application.hpp"

#include "config.hpp"
#include "fd_simulation.hpp"
#include "loading_screen.hpp"
#include "opensim_wrapper.hpp"
#include "splash_screen.hpp"

#include <OpenSim/Simulation/Model/Model.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

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

static bool safe_parse_double(char const* s, double* out) {
    char* end;
    double v = strtod(s, &end);

    if (*end != '\0' or v >= HUGE_VAL or v <= -HUGE_VAL) {
        return false;  // not a number
    }

    *out = v;
    return true;
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
        std::filesystem::path geometry_dir = osmv::config::resource_path("geometry");
        OpenSim::ModelVisualizer::addDirToGeometrySearchPaths(geometry_dir.string());
        osmv::init_application();
    }

    // no args: show splash screen
    if (argc <= 0) {
        osmv::app().start_render_loop<osmv::Splash_screen>();
        return EXIT_SUCCESS;
    }

    // no subcommand command (but args): show the UI
    //
    // the reason the subcommands are designed this way (rather than having a separate 'gui'
    // subcommand) is because most OS desktop managers call `binary.exe <arg>` when users click on
    // a file in the OS's file explorer
    osmv::app().start_render_loop<osmv::Loading_screen>(argv[0]);

    return EXIT_SUCCESS;
}
