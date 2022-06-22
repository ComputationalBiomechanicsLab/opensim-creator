#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace osc {
    // recursively find all files in `root` with any of the given extensions and
    // append the found files into the output vector
    void FindAllFilesWithExtensionsRecursively(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n,
        std::vector<std::filesystem::path>& appendOut);

    // recursively find all files in `root` with any of the given extensions and
    // return those files in a vector
    std::vector<std::filesystem::path> FindAllFilesWithExtensionsRecursively(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n);

    // recursively find all files in `root` with any of the given extensions and
    // return those files in a vector (convenience form)
    template<typename... Exensions>
    inline std::vector<std::filesystem::path> FindAllFilesWithExtensionsRecursively(
        std::filesystem::path const& root,
        Exensions&&... exts)
    {
        std::string_view extensions[] = {std::forward<Exensions>(exts)...};
        std::vector<std::filesystem::path> rv;
        FindAllFilesWithExtensionsRecursively(root, static_cast<std::string_view const*>(extensions), sizeof...(exts), rv);
        return rv;
    }

    // recursively find all files in the supplied (root) directory and return
    // them in a vector
    std::vector<std::filesystem::path> GetAllFilesInDirRecursively(std::filesystem::path const&);

    // slurp a file's contents into a string
    std::string SlurpFileIntoString(std::filesystem::path const&);

    // returns the given path's filename without an extension (e.g. /dir/model.osim --> model)
    std::string FileNameWithoutExtension(std::filesystem::path const&);
}
