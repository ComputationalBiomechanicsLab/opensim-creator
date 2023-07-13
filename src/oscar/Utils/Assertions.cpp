#include "Assertions.hpp"

#include "oscar/Platform/Log.hpp"
#include "oscar/Utils/SynchronizedValue.hpp"

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

void osc::OnAssertionFailure(
    CStringView failing_code,
    CStringView func,
    CStringView file,
    unsigned int line) noexcept
{
    auto& buf = GetGlobalAssertionErrorBuffer();
    auto guard = buf.lock();

    std::snprintf(guard->data(), guard->size(), "%s:%s:%u: assert(%s): failed", file.c_str(), func.c_str(), line, failing_code.c_str());
    log::error("%s", guard->data());
    std::terminate();
}

void osc::OnThrowingAssertionFailure(
    CStringView failingCode,
    CStringView func,
    CStringView file,
    unsigned int line)
{
    auto& buf = GetGlobalAssertionErrorBuffer();
    auto guard = buf.lock();

    std::snprintf(guard->data(), guard->size(), "%s:%s:%u: throw_if_not(%s): failed", file.c_str(), func.c_str(), line, failingCode.c_str());
    log::error("%s", guard->data());
    throw std::runtime_error{guard->data()};
}
