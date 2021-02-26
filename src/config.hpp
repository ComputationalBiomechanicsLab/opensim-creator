#pragma once

#include <chrono>
#include <filesystem>
#include <utility>
#include <vector>

namespace osmv::config {
    std::filesystem::path resource_path(std::filesystem::path const&);

    template<typename... Els>
    std::filesystem::path resource_path(Els... els) {
        std::filesystem::path p;
        (p /= ... /= els);
        return resource_path(p);
    }

    std::filesystem::path shader_path(char const* shader_name);

    struct Recent_file final {
        bool exists;
        std::chrono::seconds last_opened_unix_timestamp;
        std::filesystem::path path;

        Recent_file(bool _exists, std::chrono::seconds _last_opened_unix_timestamp, std::filesystem::path _path) :
            exists{_exists},
            last_opened_unix_timestamp{std::move(_last_opened_unix_timestamp)},
            path{std::move(_path)} {
        }
    };

    std::vector<Recent_file> recent_files();

    // persist a recently used file
    //
    // duplicates paths are automatically removed on insertion
    void add_recent_file(std::filesystem::path const&);

    bool should_use_multi_viewport();
}
