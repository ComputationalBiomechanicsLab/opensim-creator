#include "src/OpenSimBindings/Screens/MainUIScreen.hpp"
#include "src/OpenSimBindings/OpenSimApp.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/Log.hpp"
#include "src/Tabs/Tab.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Tabs/TabRegistry.hpp"

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

    // init top-level application state
    osc::OpenSimApp app;

    // init top-level screen (tab host)
    auto screen = std::make_unique<osc::MainUIScreen>(std::vector<std::filesystem::path>(argv, argv + argc));

    // if the config requested that an initial tab should be opened, try looking it up
    // and loading it
    if (auto maybeRequestedTab = app.getConfig().getInitialTabOverride())
    {
        if (auto registeredTab = osc::GetRegisteredTabByName(*maybeRequestedTab))
        {
            osc::TabHost* api = screen->getTabHostAPI();
            std::unique_ptr<osc::Tab> initialTab = registeredTab->createTab(api);
            api->selectTab(api->addTab(std::move(initialTab)));
        }
        else
        {
            osc::log::warn("%s: cannot find a tab with this name in the tab registry: ignoring", maybeRequestedTab->c_str());
            osc::log::warn("available tabs are:");
            for (int i = 0, len = osc::GetNumRegisteredTabs(); i < len; ++i)
            {
                osc::log::warn("    %s", osc::GetRegisteredTab(i).getName().c_str());
            }
        }
    }

    // enter main application loop
    app.show(std::move(screen));

    return EXIT_SUCCESS;
}
