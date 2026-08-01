#include "assertions.h"

#include <liboscar/utilities/exception_helpers.h>

#include <string_view>

void osc::detail::on_assertion_failure(
    std::string_view failing_code,
    std::string_view function_name,
    std::string_view file_name,
    unsigned int file_line)
{
    throw formatted_runtime_error(
        "{}:{}:{}: OSC_ASSERT({}) failed",
        file_name,
        function_name,
        file_line,
        failing_code
    );
}
