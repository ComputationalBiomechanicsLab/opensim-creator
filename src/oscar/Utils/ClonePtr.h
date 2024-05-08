#pragma once

#include <concepts>
#include <iosfwd>
#include <memory>
#include <type_traits>
#include <utility>

namespace osc
{
    // `ClonePtr` is a smart pointer that owns and manages another object through
    // a pointer and disposes of that object when `ClonePtr` goes out of scope.
    //
    // This is essentially the same as how `std::unique_ptr` works. The main difference
    // that `ClonePtr` offers is that it is copyable. Copying is achieved by calling a
    // cloning function when necessary.
    template<typename T, typename Deleter = std::default_delete<T>>
    class ClonePtr final {
    public:
        using pointer = T*;
        using element_type = T;
        using deleter_type = Deleter;

        // constructs a `ClonePtr` that owns nothing
        constexpr ClonePtr() noexcept : value_{nullptr} {}

        // constructs a `ClonePtr` that owns nothing
        constexpr ClonePtr(std::nullptr_t) noexcept : value_{nullptr} {}

        // constructs a `ClonePtr` that owns `p`
        explicit ClonePtr(pointer p) noexcept : value_{p} {}

        // constructs a `ClonePtr` that owns `p`
        explicit ClonePtr(std::unique_ptr<element_type, deleter_type> p) noexcept : value_{std::move(p)} {}

        // constructs a `ClonePtr` by `clone`ing `ref`
        explicit ClonePtr(const element_type& ref) : value_{ref.clone()} {}

        // constructs a `ClonePtr` by `clone`ing `src`
        ClonePtr(const ClonePtr& src) : value_{src.value_ ? src.value_->clone() : nullptr} {}

        // constructs `ClonePtr` by transferring ownership from an rvalue
        ClonePtr(ClonePtr&&) noexcept = default;

        // constructs `ClonePtr` by transferring ownership from an rvalue (with conversion)
        template<typename U, typename E>
        requires
            std::convertible_to<pointer, typename ClonePtr<U, E>::pointer> and
            (not std::is_array_v<U>) and
            std::convertible_to<E, deleter_type>
        ClonePtr(ClonePtr<U, E>&& tmp) noexcept : value_{std::move(tmp)} {}

        // if `get() == nullptr` there are no effects. Otherwise, the owned object is destroyed via `get_deleter()(get())`
        ~ClonePtr() noexcept = default;

        // if `rhs.get() != nullptr`, assigns `rhs.clone()` to `this`; otherwise, `reset`s `this`
        ClonePtr& operator=(ClonePtr const& rhs)
        {
            if (&rhs == this) {
            }
            else if (rhs.get()) {
                value_ = std::unique_ptr<element_type>{rhs.value_->clone()};
            }
            else {
                reset();
            }
            return *this;
        }

        // assigns `rhs.clone()` to `this`
        ClonePtr& operator=(const element_type& rhs)
        {
            if (value_) {
                *value_ = rhs;
            }
            else {
                value_ = std::unique_ptr<element_type>{rhs.clone()};
            }
            return *this;
        }

        ClonePtr& operator=(ClonePtr&&) noexcept = default;

        template<typename U, typename E>
        requires
            (not std::is_array_v<U>) and
            std::convertible_to<typename ClonePtr<U, E>::element_type, element_type> and
            std::is_assignable_v<Deleter&, E&&>
        ClonePtr& operator=(ClonePtr<U, E>&& rhs) noexcept
        {
            value_ = std::move(rhs.value_);
            return *this;
        }

        ClonePtr& operator=(std::nullptr_t) noexcept
        {
            reset();
            return *this;
        }

        pointer release() noexcept
        {
            return value_.release();
        }

        void reset(pointer ptr = pointer()) noexcept
        {
            value_.reset(ptr);
        }

        void swap(ClonePtr& other) noexcept
        {
            value_.swap(other.value_);
        }

        pointer get() const noexcept
        {
            return value_.get();
        }

        Deleter& get_deleter() noexcept
        {
            return value_.get_deleter();
        }

        const Deleter& get_deleter() const noexcept
        {
            return value_.get_deleter();
        }

        explicit operator bool() const noexcept
        {
            return value_;
        }

        typename std::add_lvalue_reference<T>::type operator*() const noexcept(noexcept(*std::declval<pointer>()))
        {
            return *value_;
        }

        pointer operator->() const noexcept
        {
            return value_.get();
        }

        friend void swap(ClonePtr& lhs, ClonePtr& rhs) noexcept
        {
            lhs.swap(rhs);
        }

        template<typename T2, typename D2>
        friend bool operator==(const ClonePtr& lhs, const ClonePtr<T2, D2>& rhs)
        {
            return lhs.get() == rhs.get();
        }

        template<typename CharT, typename Traits>
        friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const ClonePtr& p)
        {
            return os << p.get();
        }

    private:
        std::unique_ptr<T, Deleter> value_;
    };
}

template<typename T, typename Deleter>
struct std::hash<osc::ClonePtr<T, Deleter>> final {
    size_t operator()(const osc::ClonePtr<T, Deleter>& p) const
    {
        return std::hash(p.get());
    }
};
