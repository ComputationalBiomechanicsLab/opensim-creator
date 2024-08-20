#include <oscar/Utils/Conversion.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(Converter, automatically_defined_for_language_type_thats_implicitly_convertable)
{
    const auto converter = Converter<float, double>{};
    ASSERT_EQ(converter(5.0f), double(5.0f));
}

TEST(Converter, automatically_defined_if_implicit_construction_already_defined)
{
    struct A {};
    struct B { B(A) {} };  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)

    Converter<A, B>{}(A{});  // should compile
}

TEST(Converter, automatically_defined_if_explicit_construction_already_defined)
{
    struct A {};
    struct B { explicit B(A) {} };

    Converter<A, B>{}(A{});  // should compile
}

TEST(Converter, automatically_defined_if_user_defined_conversion_operator_defined)
{
    struct B {};
    struct A { operator B() const { return B{}; } };  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)

    Converter<A, B>{}(A{});  // should compile
}

TEST(Converter, automatically_defined_implementation_uses_ideal_constructor)
{
    enum class ConstructionMethod { Copy, Move };
    struct A {};
    struct B {
        explicit B(A&&) : method{ConstructionMethod::Move} {}
        explicit B(const A&) : method{ConstructionMethod::Copy} {}

        ConstructionMethod method;
    };

    const A lvalue;
    ConstructionMethod lvalue_method = Converter<A, B>{}(lvalue).method;
    ASSERT_EQ(lvalue_method, ConstructionMethod::Copy);
    ConstructionMethod rvalue_method = Converter<A, B>{}(A{}).method;
    ASSERT_EQ(rvalue_method, ConstructionMethod::Move);
}

TEST(Converter, to_correctly_uses_lvalues_and_rvalues)
{
    enum class ConstructionMethod { Copy, Move };
    struct A {};
    struct B {
        explicit B(A&&) : method{ConstructionMethod::Move} {}
        explicit B(const A&) : method{ConstructionMethod::Copy} {}

        ConstructionMethod method;
    };

    const A lvalue;
    ASSERT_EQ(to<B>(lvalue).method, ConstructionMethod::Copy);
    ASSERT_EQ(to<B>(A{}).method, ConstructionMethod::Move);
}
