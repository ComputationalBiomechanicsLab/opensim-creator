#pragma once

#include <concepts>
#include <utility>

namespace osc
{
    template<std::default_initializable T>
    class DefaultConstructOnCopy final {
    public:
        DefaultConstructOnCopy() = default;

        template<typename... Args>
        requires std::constructible_from<T, Args&&...>
        explicit DefaultConstructOnCopy(Args&& ...args) :
            value_{std::forward<Args>(args)...}
        {}

        DefaultConstructOnCopy(const DefaultConstructOnCopy&) :
            value_{}
        {}

        DefaultConstructOnCopy(DefaultConstructOnCopy&&) noexcept = default;

        DefaultConstructOnCopy& operator=(const DefaultConstructOnCopy&)
        {
            value_ = T{};  // exception safety: construct it then move-assign it
            return *this;
        }

        DefaultConstructOnCopy& operator=(DefaultConstructOnCopy&&) noexcept = default;

        ~DefaultConstructOnCopy() noexcept = default;

        T* operator->() { return &value_; }
        const T* operator->() const { return &value_; }
        T& operator*() { return value_; }
        const T& operator*() const { return value_; }
        T* get() { return value_; }
        const T* get() const { return value_; }

        void reset() { value_ = T{}; }

    private:
        T value_;
    };
}
