#pragma once

#include <filesystem>
#include <string_view>
#include <cstddef>
#include <vector>
#include <string>

namespace osc {
    // recursively find all files in `root` with any of the given extensions and
    // append the found files into the output vector
    void find_files_with_extensions(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n,
        std::vector<std::filesystem::path>& append_out);

    // recursively find all files in `root` with any of the given extensions and
    // return those files in a vector
    std::vector<std::filesystem::path> find_files_with_extensions(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n);

    // recursively find all files in `root` with any of the given extensions and
    // return those files in a vector (convenience form)
    template<typename... Exensions>
    inline std::vector<std::filesystem::path> find_files_with_extensions(
        std::filesystem::path const& root,
        Exensions&&... exts) {

        std::string_view extensions[] = {std::forward<Exensions>(exts)...};
        std::vector<std::filesystem::path> rv;
        find_files_with_extensions(root, static_cast<std::string_view const*>(extensions), sizeof...(exts), rv);
        return rv;
    }

    // recursively find all files in the supplied (root) directory and return
    // them in a vector
    std::vector<std::filesystem::path> files_in(std::filesystem::path const& root);

    // slurp a file's contents into a string
    std::string slurp_into_string(std::filesystem::path const&);
}
