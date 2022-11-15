#include "Assertions.hpp"

#include "src/Platform/Log.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <array>
#include <cstdio>
#include <stdexcept>

namespace
{
    // returns a static, global, buffer that assertion error messages can be formatted into
    //
    // the reason this is necessary is because we want to avoid memory allocations during
    // assertion errors
    osc::SynchronizedValue<std::array<char, 2048>>& GetGlobalAssertionErrorBuffer()
    {
        static osc::SynchronizedValue<std::array<char, 2048>> s_MessageBuffer{};
        return s_MessageBuffer;
    }
}

void osc::OnAssertionFailure(char const* failing_code,
                             char const* func,
                             char const* file,
                             unsigned int line) noexcept
{
    auto& buf = GetGlobalAssertionErrorBuffer();
    auto guard = buf.lock();

    std::snprintf(guard->data(), guard->size(), "%s:%s:%u: assert(%s): failed", file, func, line, failing_code);
    log::error("%s", guard->data());
    std::terminate();
}

void osc::OnThrowingAssertionFailure(
    char const* failingCode,
    char const* func,
    char const* file,
    unsigned int line)
{
    auto& buf = GetGlobalAssertionErrorBuffer();
    auto guard = buf.lock();

    std::snprintf(guard->data(), guard->size(), "%s:%s:%u: throw_if_not(%s): failed", file, func, line, failingCode);
    log::error("%s", guard->data());
    throw std::runtime_error{guard->data()};
}
