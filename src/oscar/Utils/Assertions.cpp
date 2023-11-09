#include "Assertions.hpp"

#include <oscar/Platform/Log.hpp>

#include <exception>
#include <sstream>
#include <stdexcept>
#include <string_view>

void osc::OnAssertionFailure(
    std::string_view failingCode,
    std::string_view func,
    std::string_view file,
    unsigned int line)
{
    std::string const msg = [&]()
    {
        std::stringstream ss;
        ss << file << ':' << func << ':' << ':' << line << ": throw_if_not(" << failingCode << ") failed";
        return std::move(ss).str();
    }();

    log::error("%s", msg.c_str());
    throw std::runtime_error{msg};
}
