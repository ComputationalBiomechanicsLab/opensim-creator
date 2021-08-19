#include "fs.hpp"

#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>

void osc::find_files_with_extensions(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n,
        std::vector<std::filesystem::path>& append_out) {

    if (!std::filesystem::exists(root)) {
        return;
    }

    if (!std::filesystem::is_directory(root)) {
        return;
    }

    for (std::filesystem::directory_entry const& e : std::filesystem::recursive_directory_iterator{root}) {
        for (size_t i = 0; i < n; ++i) {
            if (e.path().extension() == extensions[i]) {
                append_out.push_back(e.path());
            }
        }
    }
}

std::vector<std::filesystem::path> osc::find_files_with_extensions(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n) {
    std::vector<std::filesystem::path> rv;
    find_files_with_extensions(root, extensions, n, rv);
    return rv;
}

std::string osc::slurp_into_string(std::filesystem::path const& p) {
    std::ifstream f;
    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    f.open(p, std::ios::binary | std::ios::in);

    std::stringstream ss;
    ss << f.rdbuf();

    return std::move(ss).str();
}
