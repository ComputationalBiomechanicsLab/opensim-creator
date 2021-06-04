#pragma once

#include "src/config.hpp"

#include <filesystem>
#include <string_view>
#include <cstddef>
#include <vector>
#include <chrono>
#include <string>

// resources: helpers for loading data files at runtime

namespace osc {
    // get path to a runtime resource
    [[nodiscard]] inline std::filesystem::path resource(std::filesystem::path const& p) {
        return config().resource_dir.value / p;
    }

    // recursively find all files in `root` with any of the given extensions
    void find_files_with_extensions(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n,
        std::vector<std::filesystem::path>& append_out);

    // convenience form of the above
    [[nodiscard]] inline std::vector<std::filesystem::path> find_files_with_extensions(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n) {

        std::vector<std::filesystem::path> rv;
        find_files_with_extensions(root, extensions, n, rv);
        return rv;
    }

    // convenience form of the above
    template<typename... ExensionStrings>
    [[nodiscard]] inline std::vector<std::filesystem::path> find_files_with_extensions(
        std::filesystem::path const& root, ExensionStrings&&... exts) {

        std::string_view extensions[] = {std::forward<ExensionStrings>(exts)...};
        std::vector<std::filesystem::path> rv;
        find_files_with_extensions(root, static_cast<std::string_view const*>(extensions), sizeof...(exts), rv);
        return rv;
    }

    // slurp a file into a std::string
    [[nodiscard]] std::string slurp(std::filesystem::path const&);

    // slurp a resource (file) into a std::string
    [[nodiscard]] inline std::string slurp_resource(std::filesystem::path const& p) {
        return slurp(resource(p));
    }

    // definition of a recent file that was opened in the UI
    //
    // the implementation persists this information on the user's filesystem
    // so that it is remembered between boots
    struct Recent_file final {

        // whether the file actually exists n the filesystem
        bool exists;

        // when the file was last opened in OSC
        std::chrono::seconds last_opened_unix_timestamp;

        // full absolute path to the file
        std::filesystem::path path;

        Recent_file(bool _exists, std::chrono::seconds _last_opened_unix_timestamp, std::filesystem::path _path) :
            exists{_exists},
            last_opened_unix_timestamp{std::move(_last_opened_unix_timestamp)},
            path{std::move(_path)} {
        }
    };

    // returns a sequence of (usually, model) files that were recently opened in OSC
    [[nodiscard]] std::vector<Recent_file> recent_files();

    // add a path to the recent file list
    //
    // the implementation will persist this addition to the filesystem accordingly
    void add_recent_file(std::filesystem::path const&);
}
