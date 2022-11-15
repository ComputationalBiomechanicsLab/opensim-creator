#include "Assertions.hpp"

#include "src/Platform/Log.hpp"

#include <array>
#include <cstdio>
#include <stdexcept>

// allocate this globally and ahead of time, because:
//
// - using the memory allocator during an assertion failure might trigger other problems
// - stack-allocating a large buffer can cause other issues (you don't know how much stack space you have)
static std::array<char, 2048> g_MessageBuffer{};

void osc::OnAssertionFailure(char const* failing_code,
                             char const* func,
                             char const* file,
                             unsigned int line) noexcept
{
    std::snprintf(g_MessageBuffer.data(), g_MessageBuffer.size(), "%s:%s:%u: assert(%s): failed", file, func, line, failing_code);
    log::error("%s", g_MessageBuffer.data());
    std::terminate();
}

void osc::OnThrowingAssertionFailure(
    char const* failingCode,
    char const* func,
    char const* file,
    unsigned int line)
{
    std::snprintf(g_MessageBuffer.data(), g_MessageBuffer.size(), "%s:%s:%u: throw_if_not(%s): failed", file, func, line, failingCode);
    log::error("%s", g_MessageBuffer.data());
    throw std::runtime_error{g_MessageBuffer.data()};
}
