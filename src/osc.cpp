#include "src/Screens/Experimental/MathExperimentsScreen.hpp"
#include "src/Screens/Experimental/SceneGeneratorNewScreen.hpp"
#include "src/Screens/LoadingScreen.hpp"
#include "src/Screens/SplashScreen.hpp"
#include "src/App.hpp"
#include "src/Log.hpp"
#include "src/MainEditorState.hpp"

using namespace osc;

static const char g_Usage[] = R"(usage: osc [--help] [fd] MODEL.osim
)";

static const char g_Help[] = R"(OPTIONS
    --help
        Show this help
)";

static bool skipPrefix(char const* prefix, char const* s, char const** out) {
    do {
        if (*prefix == '\0' && (*s == '\0' || *s == '=')) {
            *out = s;
            return true;
        }
    } while (*prefix++ == *s++);

    return false;
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

        if (skipPrefix("--help", arg, &arg)) {
            std::cout << g_Usage << '\n' << g_Help << '\n';
            return EXIT_SUCCESS;
        }

        ++argv;
        --argc;
    }

    try {
        // init main app (window, OpenGL, etc.)
        App app;

        if (argc <= 0) {
            //app.show<SplashScreen>();
            app.show<SceneGeneratorNewScreen>();
        } else {
            auto mes = std::make_shared<MainEditorState>();
            app.show<LoadingScreen>(mes, argv[0]);
        }
    } catch (std::exception const& ex) {
        log::error("osc: encountered fatal exception: %s", ex.what());
        log::error("osc: terminating due to fatal exception");
        throw;
    }

    log::info("exited main application event loop: shutting down application");

    return EXIT_SUCCESS;
}
