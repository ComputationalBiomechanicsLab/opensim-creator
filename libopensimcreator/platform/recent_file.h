#pragma once

#include <chrono>
#include <filesystem>

namespace osc
{
    struct RecentFile final {

        // true if exists in the filesystem
        bool exists;

        // unix timestamp of when it was last opened
        std::chrono::seconds lastOpenedUnixTimestamp;

        // absolute path to the file
        std::filesystem::path path;
    };
}
