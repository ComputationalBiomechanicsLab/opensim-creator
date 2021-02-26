#pragma once

#include <mutex>

namespace osmv {
    // a mutex guard over a reference to `T`
    template<typename T>
    class Mutex_guard final {
        std::lock_guard<std::mutex> guard;
        T& ref;

    public:
        explicit Mutex_guard(std::mutex& mutex, T& _ref) : guard{mutex}, ref{_ref} {
        }

        T& operator*() noexcept {
            return ref;
        }

        T const& operator*() const noexcept {
            return ref;
        }

        T* operator->() noexcept {
            return &ref;
        }

        T const* operator->() const noexcept {
            return &ref;
        }
    };

    // represents a `T` value that can only be accessed via a mutex guard
    template<typename T>
    class Mutex_guarded {
        std::mutex mutex;
        T value;

    public:
        explicit Mutex_guarded(T _value) : value{std::move(_value)} {
        }

        // in-place constructor for T
        template<typename... Args>
        explicit Mutex_guarded(Args... args) : value{std::forward<Args...>(args)...} {
        }

        Mutex_guard<T> lock() {
            return Mutex_guard<T>{mutex, value};
        }
    };
}
