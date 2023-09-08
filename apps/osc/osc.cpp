#include "OpenSimCreator/Screens/MainUIScreen.hpp"
#include "OpenSimCreator/OpenSimCreatorApp.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace
{
    constexpr std::string_view c_Usage = "usage: osc [--help] [fd] MODEL.osim\n";

    constexpr std::string_view c_Help = R"(OPTIONS
    --help
        Show this help
)";
}

int main(int argc, char* argv[])
{
    std::vector<std::string_view> unnamedArgs;
    for (int i = 1; i < argc; ++i)
    {
        std::string_view const arg{argv[i]};  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

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
    osc::OpenSimCreatorApp app;

    // init top-level screen (tab host)
    auto screen = std::make_unique<osc::MainUIScreen>();

    // load each unnamed arg as a file in the UI
    for (auto const& unnamedArg : unnamedArgs)
    {
        screen->open(unnamedArg);
    }

    // enter main application loop
    app.show(std::move(screen));

    return EXIT_SUCCESS;
}
