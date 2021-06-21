#pragma once

#include "src/log.hpp"

#include <filesystem>
#include <string_view>

// os: where all the icky OS/distro/filesystem-specific stuff is hidden
namespace osc {
    // returns the full path to the currently-executing application
    std::filesystem::path const& current_exe_dir();

    // returns the full path to the user's data directory
    std::filesystem::path const& user_data_dir();

    // writes a backtrace for the calling thread's stack to the log at the specified level
    void write_backtrace_to_log(log::level::Level_enum);

    // installs a signal handler that prints a backtrace
    //
    // note: this is a noop on some OSes
    void install_backtrace_handler();

    // tries to open the specified filepath in the OSes default application for that
    // path. This function returns immediately: the application is opened in a separate
    // window.
    //
    // how, or what, the OS does is implementation-defined. E.g. Windows opens
    // filesystem paths by searching the file's extension against a list of default
    // applications. It opens URLs in the default browser, etc.
    void open_path_in_default_application(std::filesystem::path const&);

    // tries to open the specified URL in the OSes default browser
    //
    // how, or what, the OS does is implementation defined.
    void open_url_in_default_browser(std::string_view);
}
