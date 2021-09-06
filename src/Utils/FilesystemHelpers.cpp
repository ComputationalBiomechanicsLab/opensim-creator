#include "FilesystemHelpers.hpp"

#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>

void osc::FindAllFilesWithExtensionsRecursively(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n,
        std::vector<std::filesystem::path>& appendOut) {

    if (!std::filesystem::exists(root)) {
        return;
    }

    if (!std::filesystem::is_directory(root)) {
        return;
    }

    for (std::filesystem::directory_entry const& e : std::filesystem::recursive_directory_iterator{root}) {
        for (size_t i = 0; i < n; ++i) {
            if (e.path().extension() == extensions[i]) {
                appendOut.push_back(e.path());
            }
        }
    }
}

std::vector<std::filesystem::path> osc::FindAllFilesWithExtensionsRecursively(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n) {

    std::vector<std::filesystem::path> rv;
    FindAllFilesWithExtensionsRecursively(root, extensions, n, rv);
    return rv;
}

std::vector<std::filesystem::path> osc::GetAllFilesInDirRecursively(std::filesystem::path const& root) {
    std::vector<std::filesystem::path> rv;

    if (!std::filesystem::exists(root)) {
        return rv;
    }

    if (!std::filesystem::is_directory(root)) {
        return rv;
    }

    for (std::filesystem::directory_entry const& e : std::filesystem::recursive_directory_iterator{root}) {
        if (e.is_directory() || e.is_other() || e.is_socket()) {
            continue;
        }

        rv.push_back(e.path());
    }

    return rv;
}

std::string osc::SlurpFileIntoString(std::filesystem::path const& p) {
    std::ifstream f;
    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    f.open(p, std::ios::binary | std::ios::in);

    std::stringstream ss;
    ss << f.rdbuf();

    return std::move(ss).str();
}
