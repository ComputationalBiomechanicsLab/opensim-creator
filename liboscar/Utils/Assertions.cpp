#include "Assertions.h"

#include <sstream>
#include <stdexcept>
#include <string_view>

void osc::detail::on_assertion_failure(
    std::string_view failing_code,
    std::string_view function_name,
    std::string_view file_name,
    unsigned int file_line)
{
    std::stringstream ss;
    ss << file_name << ':' << function_name << ':' << ':' << file_line << ": OSC_ASSERT(" << failing_code << ") failed";
    throw std::runtime_error{std::move(ss).str()};
}
