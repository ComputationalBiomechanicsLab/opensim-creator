#include "src/OpenSimBindings/Screens/MainUIScreen.hpp"
#include "src/OpenSimBindings/OpenSimApp.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/Log.hpp"
#include "src/Tabs/Tab.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Tabs/TabRegistry.hpp"
#include "src/Utils/CStringView.hpp"

#include <cstdlib>
#include <iostream>

static osc::CStringView constexpr c_Usage = R"(usage: osc [--help] [fd] MODEL.osim
)";

static osc::CStringView constexpr c_Help = R"(OPTIONS
    --help
        Show this help
)";

namespace
{
    bool SkipPrefix(char const* prefix, char const* s, char const** out)
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
            std::cout << c_Usage << '\n' << c_Help << '\n';
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
    if (std::optional<std::string> maybeRequestedTab = app.getConfig().getInitialTabOverride())
    {
        std::shared_ptr<osc::TabRegistry> tabs = osc::App::singleton<osc::TabRegistry>();

        if (std::optional<osc::TabRegistryEntry> maybeEntry = tabs->getByName(*maybeRequestedTab))
        {
            std::weak_ptr<osc::TabHost> api = screen->getTabHostAPI();
            std::unique_ptr<osc::Tab> initialTab = maybeEntry->createTab(api);
            api.lock()->selectTab(api.lock()->addTab(std::move(initialTab)));
        }
        else
        {
            osc::log::warn("%s: cannot find a tab with this name in the tab registry: ignoring", maybeRequestedTab->c_str());
            osc::log::warn("available tabs are:");
            for (size_t i = 0; i < tabs->size(); ++i)
            {
                osc::log::warn("    %s", (*tabs)[i].getName().c_str());
            }
        }
    }

    // enter main application loop
    app.show(std::move(screen));

    return EXIT_SUCCESS;
}
