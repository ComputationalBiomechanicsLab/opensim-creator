#include "Assertions.h"

#include <oscar/Platform/Log.h>

#include <sstream>
#include <stdexcept>
#include <string_view>

void osc::OnAssertionFailure(
    std::string_view failing_code,
    std::string_view function_name,
    std::string_view file_name,
    unsigned int file_line)
{
    const std::string error_message = [&]()
    {
        std::stringstream ss;
        ss << file_name << ':' << function_name << ':' << ':' << file_line << ": OSC_ASSERT(" << failing_code << ") failed";
        return std::move(ss).str();
    }();

    log_error("%s", error_message.c_str());
    throw std::runtime_error{error_message};
}
