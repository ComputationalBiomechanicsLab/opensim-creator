#include "src/OpenSimBindings/OpenSimApp.hpp"
#include "src/Screens/MainUIScreen.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Tabs/Experiments/RendererHelloTriangleTab.hpp"
#include "src/Tabs/Experiments/RendererTexturingTab.hpp"
#include "src/Tabs/Experiments/RendererCoordinateSystemsTab.hpp"
#include "src/Tabs/Experiments/RendererBasicLightingTab.hpp"
#include "src/Tabs/Experiments/RendererLightingMapsTab.hpp"
#include "src/Tabs/Experiments/RendererMultipleLightsTab.hpp"
#include "src/Tabs/Experiments/RendererBlendingTab.hpp"
#include "src/Tabs/Experiments/RendererFramebuffersTab.hpp"
#include "src/Tabs/Experiments/RendererGeometryShaderTab.hpp"
#include "src/Tabs/Experiments/RendererTexturingTab.hpp"
#include "src/Tabs/Experiments/RendererOpenSimTab.hpp"

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
        auto screen = std::make_unique<osc::MainUIScreen>();
        screen->addTab(std::make_unique<osc::RendererHelloTriangleTab>(screen->getTabHostAPI()));
        screen->addTab(std::make_unique<osc::RendererTexturingTab>(screen->getTabHostAPI()));
        screen->addTab(std::make_unique<osc::RendererCoordinateSystemsTab>(screen->getTabHostAPI()));
        screen->addTab(std::make_unique<osc::RendererBasicLightingTab>(screen->getTabHostAPI()));
        screen->addTab(std::make_unique<osc::RendererLightingMapsTab>(screen->getTabHostAPI()));
        screen->addTab(std::make_unique<osc::RendererMultipleLightsTab>(screen->getTabHostAPI()));
        screen->addTab(std::make_unique<osc::RendererBlendingTab>(screen->getTabHostAPI()));
        screen->addTab(std::make_unique<osc::RendererFramebuffersTab>(screen->getTabHostAPI()));
        screen->addTab(std::make_unique<osc::RendererGeometryShaderTab>(screen->getTabHostAPI()));
        auto tabID = screen->addTab(std::make_unique<osc::RendererOpenSimTab>(screen->getTabHostAPI()));
        screen->getTabHostAPI()->selectTab(tabID);
        app.show(std::move(screen));
    }
    else
    {
        app.show<osc::MainUIScreen>(argv[0]);
    }

    return EXIT_SUCCESS;
}
