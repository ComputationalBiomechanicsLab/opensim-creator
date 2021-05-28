#pragma once

#include "src/config.hpp"

#include <filesystem>
#include <string_view>
#include <cstddef>
#include <vector>
#include <chrono>

namespace osc {
    // get path to a runtime resource
    [[nodiscard]] static inline std::filesystem::path resource(std::filesystem::path const& p) {
        return config().resource_dir.value / p;
    }

    // recursively find all files in `root` with any of the given extensions
    void find_files_with_extensions(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n,
        std::vector<std::filesystem::path>& append_out);

    // convenience form of the above
    [[nodiscard]] static inline std::vector<std::filesystem::path> find_files_with_extensions(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n) {

        std::vector<std::filesystem::path> rv;
        find_files_with_extensions(root, extensions, n, rv);
        return rv;
    }

    // convenience form of the above
    template<typename... ExensionStrings>
    [[nodiscard]] static inline std::vector<std::filesystem::path> find_files_with_extensions(
        std::filesystem::path const& root, ExensionStrings&&... exts) {

        std::string_view extensions[] = {std::forward<ExensionStrings>(exts)...};
        std::vector<std::filesystem::path> rv;
        find_files_with_extensions(root, static_cast<std::string_view const*>(extensions), sizeof...(exts), rv);
        return rv;
    }

    [[nodiscard]] std::string slurp(std::filesystem::path const&);

    [[nodiscard]] static inline std::string slurp_resource(std::filesystem::path const& p) {
        return slurp(resource(p));
    }

    // a recent file that was opened in the UI
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

    [[nodiscard]] std::vector<Recent_file> recent_files();
    void add_recent_file(std::filesystem::path const&);
}
