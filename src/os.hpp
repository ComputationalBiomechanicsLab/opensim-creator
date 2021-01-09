#pragma once

#include <filesystem>

// os: where all the icky OS/distro/filesystem-specific stuff is hidden
namespace osmv {
    // returns the full path to the currently-executing application
    std::filesystem::path current_exe_dir();
}
