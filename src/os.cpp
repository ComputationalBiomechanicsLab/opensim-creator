#include "os.hpp"

#include <SDL.h>

#include <cstring>
#include <memory>
#include <sstream>

using std::literals::string_literals::operator""s;

static std::filesystem::path get_current_exe_dir() {
    std::unique_ptr<char, decltype(&SDL_free)> p{SDL_GetBasePath(), SDL_free};

    if (not p) {
        throw std::runtime_error{"SDL_GetBasePath: returned null: "s + SDL_GetError()};
    }

    size_t l = std::strlen(p.get());

    if (l == 0) {
        throw std::runtime_error{"SDL_GetBasePath: returned an empty string"};
    }

    // remove trailing slash: it interferes with std::filesystem::path
    p.get()[l - 1] = '\0';

    return std::filesystem::path{p.get()};
}

std::filesystem::path osmv::current_exe_dir() {
    // can be expensive to compute: cache after first retrieval
    static std::filesystem::path const d = get_current_exe_dir();

    return d;
}
