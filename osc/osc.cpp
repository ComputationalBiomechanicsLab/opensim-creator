#include <osc/osc_config.h>

#include <libopensimcreator/Platform/OpenSimCreatorApp.h>
#include <libopensimcreator/UI/MainUIScreen.h>
#include <liboscar/Platform/AppMetadata.h>
#include <liboscar/UI/Tabs/TabRegistry.h>
#include <liboscar-demos/OscarDemosTabRegistry.h>

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
        AppMetadata metadata;
        metadata.set_organization_name(OSC_ORGNAME_STRING);
        metadata.set_application_name(OSC_APPNAME_STRING);
        metadata.set_config_filename("osc.toml");
        metadata.set_long_application_name(OSC_LONG_APPNAME_STRING);
        metadata.set_version_string(OSC_VERSION_STRING);
        metadata.set_build_id(OSC_BUILD_ID);
        metadata.set_repository_url(OSC_REPO_URL);
        metadata.set_help_url(OSC_HELP_URL);
        return metadata;
    }
}

int main(int argc, char* argv[])
{
    std::vector<std::string_view> unnamed_args;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        if (arg.empty()) {
            // do nothing (this shouldn't happen)
        }
        else if (arg.front() != '-') {
            unnamed_args.push_back(arg);
        }
        else if (arg == "--help") {
            std::cout << c_Usage << '\n' << c_Help << '\n';
            return EXIT_SUCCESS;
        }
    }

    // init top-level application state
    OpenSimCreatorApp app{GetOpenSimCreatorAppMetadata()};

    // also, register the demo tabs
    register_demo_tabs(app.upd_tab_registry());

    // init top-level screen (tab host)
    auto screen = std::make_unique<MainUIScreen>();

    // load each unnamed arg as a file in the UI
    for (const auto& unnamed_arg : unnamed_args) {
        screen->open(unnamed_arg);
    }

    // enter main application loop
    app.show(std::move(screen));

    return EXIT_SUCCESS;
}
