#include <osc/osc_config.h>

#include <OpenSimCreator/Platform/OpenSimCreatorApp.h>
#include <OpenSimCreator/UI/MainUIScreen.h>
#include <oscar/Platform/AppMetadata.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

using namespace osc;

namespace
{
    constexpr std::string_view c_Usage = "usage: osc [--help] [fd] MODEL.osim\n";

    constexpr std::string_view c_Help = R"(OPTIONS
    --help
        Show this help
)";

    AppMetadata GetOpenSimCreatorAppMetadata()
    {
        return AppMetadata{
            OSC_ORGNAME_STRING,
            OSC_APPNAME_STRING,
            OSC_LONG_APPNAME_STRING,
            OSC_VERSION_STRING,
            OSC_BUILD_ID,
            OSC_REPO_URL,
            OSC_HELP_URL,
        };
    }
}

int main(int argc, char* argv[])
{
    std::vector<std::string_view> unnamedArgs;
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg{argv[i]};  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        if (arg.empty())
        {
            // do nothing (this shouldn't happen)
        }
        else if (arg.front() != '-')
        {
            unnamedArgs.push_back(arg);
        }
        else if (arg == "--help")
        {
            std::cout << c_Usage << '\n' << c_Help << '\n';
            return EXIT_SUCCESS;
        }
    }

    // init top-level application state
    OpenSimCreatorApp app{GetOpenSimCreatorAppMetadata()};

    // init top-level screen (tab host)
    auto screen = std::make_unique<MainUIScreen>();

    // load each unnamed arg as a file in the UI
    for (const auto& unnamedArg : unnamedArgs) {
        screen->open(unnamedArg);
    }

    // enter main application loop
    app.show(std::move(screen));

    return EXIT_SUCCESS;
}
