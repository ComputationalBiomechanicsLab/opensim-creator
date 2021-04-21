#include "helpers.hpp"

#include <fstream>

std::string osc::slurp_into_string(std::filesystem::path const& p) {
    std::ifstream f;
    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    f.open(p, std::ios::binary | std::ios::in);

    std::stringstream ss;
    ss << f.rdbuf();

    return std::move(ss).str();
}
