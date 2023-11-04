#include "Assertions.hpp"

#include <oscar/Platform/Log.hpp>

#include <exception>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

void osc::OnAssertionFailure(
    std::string_view failingCode,
    std::source_location location)
{
    std::string const msg = [&failingCode, &location]()
    {
        std::stringstream ss;
        ss << location.file_name() << ':' << location.function_name() << ':' << location.line() << ": '" << failingCode << "' returned false";
        return std::move(ss).str();
    }();

    throw std::runtime_error{msg};
}
