#include "Assertions.h"

#include <oscar/Platform/Log.h>

#include <sstream>
#include <stdexcept>
#include <string_view>

void osc::OnAssertionFailure(
    std::string_view failingCode,
    std::string_view func,
    std::string_view file,
    unsigned int line)
{
    const std::string msg = [&]()
    {
        std::stringstream ss;
        ss << file << ':' << func << ':' << ':' << line << ": OSC_ASSERT(" << failingCode << ") failed";
        return std::move(ss).str();
    }();

    log_error("%s", msg.c_str());
    throw std::runtime_error{msg};
}
