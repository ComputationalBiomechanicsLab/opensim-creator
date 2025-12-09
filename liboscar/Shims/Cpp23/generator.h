#pragma once

#include <version>  // __cpp_lib_generator

#ifdef __cpp_lib_generator
    #include <generator>
#else
    #include <liboscar/Utils/Assertions.h>

    #include <coroutine>
    #include <cstddef>
    #include <exception>
    #include <iterator>
    #include <memory>
    #include <ranges>
    #include <stack>
    #include <type_traits>
    #include <utility>
#endif

namespace osc::cpp23
{
#ifdef __cpp_lib_generator
    template<class Ref, class V = void, class Allocator = void>
    using generator = std::generator<Ref, V, Allocator>;
#else
    // WORK IN PROGRESS
    //
    // liboscar shim for `std::generator<Ref, V, Allocator>` (and also a bit of fun,
    // because what kind of C++ programmer isn't intrigued by coroutines? ;)).
    template<class Ref, class V = void>
    class generator : public std::ranges::view_interface<generator<Ref, V>> {
    private:
        using value = std::conditional_t<std::is_void_v<V>, std::remove_cvref_t<Ref>, V>;
        using reference = std::conditional_t<std::is_void_v<V>, Ref&&, Ref>;
    public:
        using yielded = std::conditional_t<std::is_reference_v<reference>, reference, const reference&>;

        // The promise type of `generator`
        class promise_type final {
        public:
            std::add_pointer_t<yielded> value_ = nullptr;
            std::exception_ptr except_;
            std::stack<std::coroutine_handle<>>* active_ = nullptr;

            generator get_return_object() noexcept
            {
                generator generator{std::coroutine_handle<promise_type>::from_promise(*this)};
                generator.coroutine_.promise().active_ = generator.active_.get();
                return generator;
            }

            std::suspend_always initial_suspend() const noexcept { return std::suspend_always{}; }

            // Specialized awaiter: transfers control back to the parent of a nested coroutine.
            struct final_awaiter final {

                // `final_suspend` always suspends the coroutine.
                bool await_ready() noexcept { return false; }

                // This is always called (`final_suspend` always suspends).
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) noexcept
                {
                    // Pops the coroutine handle from the top of *active_.
                    // If *x.active_ is not empty, resumes execution of the coroutine referred to by x.active_->top(). If it is empty, control flow returns to the current coroutine caller or resumer.
                    auto* active = handle.promise().active_;
                    OSC_ASSERT(active != nullptr and active->top() == handle);

                    active->pop();
                    if (not active->empty()) {
                        return active->top();
                    }
                    else {
                        return std::noop_coroutine();
                    }
                }

                // never called (final_suspend always suspends)
                void await_resume() noexcept {}
            };

            final_awaiter final_suspend() noexcept { return {}; }

            std::suspend_always yield_value(yielded val) { value_ = std::addressof(val); return {}; }

            void return_void() const noexcept {}

            void unhandled_exception()
            {
                except_ = std::current_exception();
            }
        private:
        };

        // The return type of `generator::begin`. Models `indirectly_readable` and `input_iterator`.
        class iterator final {
        public:
            using value_type = value;
            using difference_type = std::ptrdiff_t;

            explicit iterator(std::coroutine_handle<promise_type> coroutine) :
                coroutine_{coroutine}
            {}

            iterator(iterator&& other) noexcept : coroutine_{std::exchange(other.coroutine_), {}} {}
            iterator& operator=(iterator&& other) noexcept { coroutine_ = std::exchange(other.coroutine_, {}); return *this; }

            reference operator*() const noexcept(std::is_nothrow_copy_constructible_v<reference>)
            {
                if (coroutine_.promise().except_) {
                    std::rethrow_exception(coroutine_.promise().except_);
                }
                // - Let `reference` be the `std::generator`'s underlying type.
                // - Let for some `generator` object `x` its coroutine_ be in the stack `*x.active_`.
                // - Let `x.active_->top()` refer to a suspended coroutine with promise object `p`
                //
                // Equivalent to `return static_cast<reference>(*p.value_);`.
                return static_cast<reference>(*coroutine_.promise().value_);
            }

            constexpr iterator& operator++()
            {
                coroutine_.promise().except_ = nullptr;
                coroutine_.resume();
                return *this;
            }

            constexpr iterator& operator++(int)
            {
                return ++*this;
            }

            friend bool operator==(const iterator& lhs, std::default_sentinel_t)
            {
                return lhs.coroutine_.done();
            }
        private:
            std::coroutine_handle<promise_type> coroutine_;
        };

        generator(const generator&) = delete;
        generator(generator&& tmp) noexcept :
            active_{std::move(tmp.active_)},
            coroutine_{std::exchange(tmp.coroutine_, {})}
        {}
        ~generator() noexcept
        {
            coroutine_.destroy();
        }

        generator& operator=(generator other) noexcept
        {
            std::swap(active_, other.active_);
            std::swap(coroutine_, other.coroutine_);
        }

        iterator begin()
        {
            OSC_ASSERT(coroutine_ and not coroutine_.done());
            active_->push(coroutine_);
            coroutine_.resume();
            return iterator{coroutine_};
        }

        std::default_sentinel_t end() const noexcept
        {
            return std::default_sentinel;
        }
    private:
        explicit generator(
            std::coroutine_handle<promise_type> coroutine) :
            coroutine_{std::move(coroutine)}
        {}

        std::unique_ptr<std::stack<std::coroutine_handle<>>> active_ = std::make_unique<std::stack<std::coroutine_handle<>>>();
        std::coroutine_handle<promise_type> coroutine_;
    };
#endif
}
