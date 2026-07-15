#include "assertions.h"

#include <format>
#include <stdexcept>
#include <string_view>
#include <utility>

void osc::detail::on_assertion_failure(
    std::string_view failing_code,
    std::string_view function_name,
    std::string_view file_name,
    unsigned int file_line)
{
    auto msg = std::format("{}:{}:{}: OSC_ASSERT({}) failed",
        file_name,
        function_name,
        file_line,
        failing_code
    );
    throw std::runtime_error{msg};
}
