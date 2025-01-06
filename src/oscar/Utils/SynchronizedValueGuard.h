#pragma once

#include <mutex>

namespace osc
{
    // accessor to a reference to the guarded value
    template<
        typename T,
        typename Mutex = std::mutex,
        typename MutexGuard = std::lock_guard<Mutex>
    >
    class SynchronizedValueGuard final {
    public:
        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using mutex_type = Mutex;
        using mutex_guard_type = MutexGuard;

        SynchronizedValueGuard(mutex_type& mutex, reference value_ref) :
            mutex_guard_{mutex},
            value_ptr_{&value_ref}
        {}

        reference operator*() & { return *value_ptr_; }
        const_reference operator*() const & { return *value_ptr_; }
        pointer operator->() { return value_ptr_; }
        const_pointer operator->() const { return value_ptr_; }

    private:
        mutex_guard_type mutex_guard_;
        pointer value_ptr_;
    };
}
