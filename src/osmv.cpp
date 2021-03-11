#include "application.hpp"

#include "src/config.hpp"
#include "src/log.hpp"
#include "src/screens/loading_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/utils/circular_buffer.hpp"
#include "src/utils/circular_log_sink.hpp"
#include "src/utils/os.hpp"

#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <OpenSim/Analyses/RegisterTypes_osimAnalyses.h>
#include <OpenSim/Common/LogSink.h>
#include <OpenSim/Common/Logger.h>
#include <OpenSim/Common/RegisterTypes_osimCommon.h>
#include <OpenSim/Simulation/Model/ModelVisualizer.h>
#include <OpenSim/Simulation/RegisterTypes_osimSimulation.h>
#include <OpenSim/Tools/RegisterTypes_osimTools.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

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

namespace {
    class Opensim_log_sink final : public OpenSim::LogSink {
        void sinkImpl(std::string const& msg) override {
            log::info("%s", msg.c_str());
        }
    };
}

int main(int argc, char** argv) {
    // skip application name
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
        // init traceback log sink
        //
        // this is an in-memory log sink that the UI can use to view messages as they
        // happen (in contrast to a file/console one, which is usually used to persist
        // all messages as part of a crash investigation)
        osmv::init_traceback_log();

        // install backtrace dumper
        //
        // useful if the application fails in prod: can provide some basic backtrace
        // info that users can paste into an issue or something, which is *a lot* more
        // information than "yeah, it's broke"
        log::info("enabling backtrace handler");
        osmv::install_backtrace_handler();

        // disable OpenSim's `opensim.log` default
        //
        // by default, OpenSim creates an `opensim.log` file in the process's working
        // directory. This should be disabled because it screws with running multiple
        // instances of the UI on filesystems that use locking (e.g. Windows) and
        // because it's incredibly obnoxious to have `opensim.log` appear in every
        // working directory from which osmv is ran
        log::info("removing OpenSim's default log (opensim.log)");
        OpenSim::Logger::removeFileSink();

        // add OSMV in-memory logger
        //
        // this logger collects the logs into a global mutex-protected in-memory structure
        // that the UI can can trivially render (w/o reading files etc.)
        log::info("attaching OpenSim to this log");
        OpenSim::Logger::addSink(std::make_shared<Opensim_log_sink>());

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
        log::info("registering OpenSim types");
        RegisterTypes_osimCommon();
        RegisterTypes_osimSimulation();
        RegisterTypes_osimActuators();
        RegisterTypes_osimAnalyses();
        RegisterTypes_osimTools();

        // globally set OpenSim's geometry search path
        //
        // when an osim file contains relative geometry path (e.g. "sphere.vtp"), the
        // OpenSim implementation will look in these directories for that file
        log::info("registering OpenSim geometry search path to use osmv resources");
        std::filesystem::path geometry_dir = osmv::config::resource_path("geometry");
        OpenSim::ModelVisualizer::addDirToGeometrySearchPaths(geometry_dir.string());
        log::info("added geometry search path entry: %s", geometry_dir.string().c_str());
    }

    // init an application instance ready for rendering
    log::info("initializing application");
    Application app;
    Application::set_current(&app);

    if (argc <= 0) {
        // no args: show splash screen
        app.start_render_loop<Splash_screen>();
    } else {
        // args: load args as osim files
        app.start_render_loop<Loading_screen>(argv[0]);
    }

    log::info("exited main application event loop: shutting down application");

    return EXIT_SUCCESS;
}
