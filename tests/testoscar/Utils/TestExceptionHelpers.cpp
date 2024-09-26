#include <oscar/Utils/ExceptionHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Utils/StringHelpers.h>

#include <stdexcept>

using namespace osc;

namespace
{
    void h()
    {
        throw std::runtime_error{"h()"};
    }

    void g()
    {
        try {
            h();
        }
        catch (const std::exception&) {
            std::throw_with_nested(std::runtime_error{"g()"});
        }
    }

    void f()
    {
        try {
            g();
        }
        catch (const std::exception&) {
            std::throw_with_nested(std::runtime_error{"f()"});
        }
    }
}

TEST(potentially_nested_exception_to_string, works_as_intended)
{
    std::string msg;
    try {
        f();
    }
    catch (const std::exception& ex) {
        msg = potentially_nested_exception_to_string(ex);
    }

    ASSERT_TRUE(contains(msg, "h()"));
    ASSERT_TRUE(contains(msg, "g()"));
    ASSERT_TRUE(contains(msg, "f()"));
}
