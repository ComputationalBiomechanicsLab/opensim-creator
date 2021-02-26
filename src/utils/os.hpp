#pragma once

#include <filesystem>

// os: where all the icky OS/distro/filesystem-specific stuff is hidden
namespace osmv {
    // returns the full path to the currently-executing application
    std::filesystem::path const& current_exe_dir();

    // returns the full path to the user's data directory
    std::filesystem::path const& user_data_dir();

    // installs a signal handler that prints a backtrace
    //
    // note: this is a noop on some OSes
    void install_backtrace_handler();
}
