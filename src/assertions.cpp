#include "assertions.hpp"

#include <cstdio>
#include <cstring>
#include <stdexcept>

void osmv::on_assertion_failure(char const* failing_code, char const* file, unsigned int line) noexcept {
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s:%u: Assertion '%s' failed", file, line, failing_code);
    try {
        throw std::runtime_error{buf};
    } catch (std::runtime_error const&) {
        std::terminate();
    }
}
