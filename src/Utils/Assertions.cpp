#include "Assertions.hpp"

#include <cstdio>
#include <stdexcept>

void osc::OnAssertionFailure(char const* failing_code,
                             char const* func,
                             char const* file,
                             unsigned int line)
{
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s:%s:%u: Assertion '%s' failed", file, func, line, failing_code);
    throw std::runtime_error{buf};
}
