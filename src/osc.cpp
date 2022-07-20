#include "src/OpenSimBindings/OpenSimApp.hpp"
#include "src/Screens/MainUIScreen.hpp"

#include <cstdlib>
#include <iostream>

static const char g_Usage[] = R"(usage: osc [--help] [fd] MODEL.osim
)";

static const char g_Help[] = R"(OPTIONS
    --help
        Show this help
)";

static bool SkipPrefix(char const* prefix, char const* s, char const** out)
{
    do
    {
        if (*prefix == '\0' && (*s == '\0' || *s == '='))
        {
            *out = s;
            return true;
        }
    }
    while (*prefix++ == *s++);

    return false;
}

int main(int argc, char** argv)
{
    // skip application name
    --argc;
    ++argv;

    // handle named flag args (e.g. --help)
    while (argc)
    {
        char const* arg = *argv;

        if (*arg != '-')
        {
            break;
        }

        if (SkipPrefix("--help", arg, &arg))
        {
            std::cout << g_Usage << '\n' << g_Help << '\n';
            return EXIT_SUCCESS;
        }

        ++argv;
        --argc;
    }

    // init main app (window, OpenGL, etc.)
    osc::OpenSimApp app;

    if (argc <= 0)
    {
        app.show<osc::MainUIScreen>();
    }
    else
    {
        app.show<osc::MainUIScreen>(argv[0]);
    }

    return EXIT_SUCCESS;
}
