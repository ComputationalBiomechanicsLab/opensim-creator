#include "ExceptionHelpers.h"

#include <ostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <utility>

namespace
{
    // copied from: https://en.cppreference.com/w/cpp/error/throw_with_nested
    void print_exception(const std::exception& ex, std::ostream& out, int indent)
    {
        out << std::string(indent, ' ') << "exception: " << ex.what() << '\n';
        try {
            std::rethrow_if_nested(ex);
        }
        catch (const std::exception& nested_exception) {
            print_exception(nested_exception, out, indent + 1);
        }
        catch (...) {
            // do nothing (stop recursing, and swallow the exception)
        }
    }
}

std::string osc::potentially_nested_exception_to_string(const std::exception& ex, int indent)
{
    std::stringstream ss;
    print_exception(ex, ss, indent);
    return std::move(ss).str();
}
