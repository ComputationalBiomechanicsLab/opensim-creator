#pragma once

#include <chrono>
#include <filesystem>

namespace osc {
    struct Recent_file final {
        bool exists;  // true if exists in the filesystem
        std::chrono::seconds unix_timestamp;  // unix timestamp of when it was last opened
        std::filesystem::path path;  // absolute path to the file
    };
}
