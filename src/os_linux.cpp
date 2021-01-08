#include "os.hpp"

#include <sstream>

#include <unistd.h>
#include <string.h>

std::filesystem::path osmv::current_exe_path() {
    char buf[2048];

    ssize_t rv = readlink("/proc/self/exe", buf, sizeof(buf));

    if (rv == -1) {
        std::stringstream ss;
        ss << "could not get path to current executable: " << strerror(errno);
        throw std::runtime_error{std::move(ss).str()};
    }

    if (rv == sizeof(buf)) {
        throw std::runtime_error{"could not get path to current executable: the path to the executable is too long"};
    }

    buf[rv] = '\0';

    return std::filesystem::path{buf};
}
