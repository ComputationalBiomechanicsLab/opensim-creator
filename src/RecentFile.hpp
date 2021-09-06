#pragma once

#include <chrono>
#include <filesystem>

namespace osc {
    struct RecentFile final {
        bool exists;  // true if exists in the filesystem
        std::chrono::seconds lastOpenedUnixTimestamp;  // unix timestamp of when it was last opened
        std::filesystem::path path;  // absolute path to the file
    };
}
