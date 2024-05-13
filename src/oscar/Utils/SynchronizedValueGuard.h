#pragma once

#include <mutex>

namespace osc
{
    // accessor to a reference to the guarded value
    template<
        typename T,
        typename TGuard = std::lock_guard<std::mutex>
    >
    class SynchronizedValueGuard final {
    public:
        SynchronizedValueGuard(std::mutex& mutex, T& value_ref) :
            mutex_guard_{mutex},
            value_ptr_{&value_ref}
        {}

        T& operator*() & { return *value_ptr_; }
        const T& operator*() const & { return *value_ptr_; }
        T* operator->() { return value_ptr_; }
        const T* operator->() const { return value_ptr_; }

    private:
        TGuard mutex_guard_;
        T* value_ptr_;
    };
}
