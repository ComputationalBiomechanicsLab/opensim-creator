#include "scope_exit.h"

#include <gtest/gtest.h>

using namespace osc;

namespace
{
    // Helper class for exercising the move-constructor behavior that's
    // documented for `std::experimental::scope_exit<EF>`, to ensure that
    // OpenSim's rewrite of it is mostly forward-compatible.
    template<bool IsNoexceptMoveConstructible, bool IsNoexceptCopyConstructible>
    class Callable final {
    public:
        explicit Callable(int* calls, int* copies, int* moves) :
            calls_{calls}, copies_{copies}, moves_{moves}
        {}
        Callable(Callable&& tmp) noexcept(IsNoexceptMoveConstructible) :
            calls_{tmp.calls_}, copies_{tmp.copies_}, moves_{tmp.moves_}
        {
            ++(*moves_);
        }
        Callable(const Callable& src) noexcept(IsNoexceptCopyConstructible) :
            calls_{src.calls_}, copies_{src.copies_}, moves_{src.moves_}
        {
            ++(*copies_);
        }
        ~Callable() noexcept = default;

        Callable& operator=(const Callable&) = delete;
        Callable& operator=(Callable&&) noexcept = delete;


        void operator()() const noexcept { ++(*calls_); }
    private:
        int* calls_;
        int* copies_;
        int* moves_;
    };
}

TEST(ScopeExit, calls_exit_function_on_normal_scope_exit)
{
    bool called = false;
    {
        ScopeExit exit{[&called]() { called = true; }};
    }
    ASSERT_TRUE(called);
}

TEST(ScopeExit, calls_exit_function_when_exception_is_thrown)
{
    bool called = false;
    {
        try {
            ScopeExit exit{[&called]() { called = true; }};
            throw std::runtime_error{"throw something"};
        }
        catch (const std::exception&) {}  // ignore the exception
    }
    ASSERT_TRUE(called);
}

TEST(ScopeExit, when_move_constructed_only_calls_exit_function_once)
{
    int calls = 0;
    {
        ScopeExit first{[&calls]() { ++calls; }};
        ScopeExit second{std::move(first)};
    }
    ASSERT_EQ(calls, 1);
}

TEST(ScopeExit, when_move_constructed_does_not_forget_release_of_source_object)
{
    int calls = 0;
    {
        ScopeExit first{[&calls]() { ++calls; }};
        first.release();
        ScopeExit second{std::move(first)};
    }
    ASSERT_EQ(calls, 0);
}

TEST(ScopeExit, copies_function_object_if_its_move_constructor_is_not_noexcept)
{
    // This test just ensures `ScopeExit` has similar semantics
    // to `std::experimental::scope_exit`.

    int calls = 0;
    int copies = 0;
    int moves = 0;
    {
        ScopeExit first{Callable<false, true>{&calls, &copies, &moves}};
        ScopeExit second{std::move(first)};
    }
    ASSERT_EQ(calls, 1);   // Destruction of `second`
    ASSERT_EQ(copies, 1);  // Copying via `ScopeExit::ScopeExit(ScopeExit&&)`
    ASSERT_EQ(moves, 1);   // Initial construction
}

TEST(ScopeExit, moves_the_exit_function_if_its_move_constructor_is_noexcept)
{
    // This test just ensures `ScopeExit` has similar semantics
    // to `std::experimental::scope_exit`.

    int calls = 0;
    int copies = 0;
    int moves = 0;
    {
        ScopeExit first{Callable<true, true>{&calls, &copies, &moves}};
        ScopeExit second{std::move(first)};
    }
    ASSERT_EQ(calls, 1);   // Destruction of `second`
    ASSERT_EQ(copies, 0);
    ASSERT_EQ(moves, 2);   // Initial construction and move construction of `ScopeExit`
}

TEST(ScopeExit, release_stops_exit_function_from_being_called_on_normal_scope_exit)
{
    bool called = false;
    {
        ScopeExit exit{[&called]() { called = true; }};
        exit.release();
    }
    ASSERT_FALSE(called);
}

TEST(ScopeExit, release_stops_exit_function_from_being_called_when_exception_is_thrown)
{
    bool called = false;
    {
        try {
            ScopeExit exit{[&called]() { called = true; }};
            exit.release();
            throw std::runtime_error{"throw something"};
        }
        catch (const std::exception&) {}  // ignore the exception
    }
    ASSERT_FALSE(called);
}
