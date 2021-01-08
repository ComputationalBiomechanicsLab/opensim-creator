#include "os.hpp"

#include <Windows.h>
#include <libloaderapi.h>

namespace fs = std::filesystem;

std::filesystem::path osmv::current_exe_path() {
    char buf[2048];
    if (not GetModuleFileNameA(nullptr, buf, sizeof(buf))) {
        throw std::runtime_error{"could not get a path to the current executable - the path may be too long? (max: 2048 chars)"};
    }
    return std::filesystem::path{buf};
}
