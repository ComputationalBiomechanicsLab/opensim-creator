#pragma once

#include <oscar/Utils/LifetimeWatcher.h>
#include <oscar/Utils/WatchableLifetime.h>

#include <cstddef>
#include <concepts>
#include <stdexcept>
#include <utility>

namespace osc
{
    // a non-owning smart pointer that ties a `LifetimeWatcher` to an unmanaged (raw)
    // pointer in order to enable lifetime checking at runtime on the pointer
    //
    // note: the main utility of this class is that it enables some basic runtime checking
    //       without having to invasively add reference counters etc. to the things being
    //       managed
    //
    // note: `LifetimedPtr` isn't threadsafe in the same way that (e.g.) `std::weak_ptr`
    //       is. Because there's no way to `lock` a raw pointer, this class is susceptible
    //       to (e.g.) checking the lifetime, followed by accessing the object while the
    //       owning thread is destructing it
    template<typename T>
    class LifetimedPtr final {
    public:
        // constructs a `nullptr` with an already-dead lifetime
        constexpr LifetimedPtr() = default;

        // constructs a `nullptr` with an already-dead lifetime
        constexpr LifetimedPtr(std::nullptr_t) : LifetimedPtr{} {}

        // constructs a `LifetimedPtr` that ties `lifetime_watcher` to `ptr`
        LifetimedPtr(LifetimeWatcher lifetime_watcher, T* ptr) :
            lifetime_watcher_{std::move(lifetime_watcher)},
            ptr_{ptr}
        {}

        // constructs a  `LifetimedPtr` that ties `lifetime` to `ptr`
        template<WatchableLifetime Lifetime>
        LifetimedPtr(const Lifetime& lifetime, T* ptr) :
            LifetimedPtr{lifetime.watch(), ptr}
        {}

        // normal copy construction (the lifetime is shared)
        LifetimedPtr(const LifetimedPtr&) = default;

        // constructs a `LifetimedPtr` by coercing from a more-derived pointer
        template<std::derived_from<T> U>
        LifetimedPtr(const LifetimedPtr<U>& other) :
            lifetime_watcher_{other.lifetime_watcher_},
            ptr_{other.ptr_}
        {}

        // normal move construction (the lifetime is moved from)
        LifetimedPtr(LifetimedPtr&&) noexcept = default;

        // constructs a `LifetimedPtr` by coercing from a more-derived pointer
        template<std::derived_from<T> U>
        LifetimedPtr(LifetimedPtr<U>&& other) :
            lifetime_watcher_{std::move(other.lifetime_watcher_)},
            ptr_{other.ptr_}
        {}

        LifetimedPtr& operator=(const LifetimedPtr&) = default;
        LifetimedPtr& operator=(LifetimedPtr&&) noexcept = default;

        ~LifetimedPtr() noexcept = default;

        bool expired() const noexcept
        {
            return lifetime_watcher_.expired();
        }

        void reset()
        {
            *this = LifetimedPtr{};
        }

        T* get() const
        {
            if (ptr_ == nullptr) {
                return nullptr;
            }
            assert_within_lifetime();
            return ptr_;
        }

        T& operator*() const
        {
            assert_within_lifetime();
            return *ptr_;
        }

        T* operator->() const
        {
            assert_within_lifetime();
            return ptr_;
        }

        operator bool() const noexcept
        {
            return ptr_ and not lifetime_watcher_.expired();
        }

        template<typename U>
        LifetimedPtr<U> dynamic_downcast()
        {
            return {lifetime_watcher_, dynamic_cast<U*>(ptr_)};
        }
    private:
        void assert_within_lifetime() const
        {
            if (lifetime_watcher_.expired()) {
                throw std::runtime_error{"attempted to dereference a null pointer"};
            }
        }

        // self-friend-class for the coercing constructor
        template<typename> friend class LifetimedPtr;

        template<typename T1, typename T2>
        friend bool operator==(const LifetimedPtr<T1>&, const LifetimedPtr<T2>&);

        template<typename T1>
        friend bool operator==(const LifetimedPtr<T1>&, std::nullptr_t);

        LifetimeWatcher lifetime_watcher_;
        T* ptr_ = nullptr;
    };

    template<typename T, typename U>
    bool operator==(const LifetimedPtr<T>& lhs, const LifetimedPtr<U>& rhs)
    {
        return lhs.ptr_ == rhs .ptr_;
    }

    template<typename T>
    bool operator==(const LifetimedPtr<T>& lhs, std::nullptr_t)
    {
        return lhs.ptr_ == nullptr;
    }
}
