#pragma once

#include <algorithm>
#include <atomic>
#include <compare>
#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace osc
{
    template<typename T>
    class CopyOnUpdSharedValue final {
    public:
        using pointer = T*;
        using const_pointer = const T*;
        using const_reference = const T&;

        CopyOnUpdSharedValue(const CopyOnUpdSharedValue& other) noexcept :
            ptr_{other.ptr_}
        {
            ptr_->owners.fetch_add(1, std::memory_order::relaxed);
        }

        ~CopyOnUpdSharedValue() noexcept
        {
            if (ptr_->owners.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                ptr_->deleter(ptr_);
            }
        }

        CopyOnUpdSharedValue& operator=(const CopyOnUpdSharedValue& rhs) noexcept
        {
            CopyOnUpdSharedValue copy{rhs};
            swap(copy, *this);
            return *this;
        }

        friend void swap(CopyOnUpdSharedValue& a, CopyOnUpdSharedValue& b) noexcept
        {
            std::swap(a.ptr_, b.ptr_);
        }

        template<typename U>
        friend bool operator==(const CopyOnUpdSharedValue& lhs, const CopyOnUpdSharedValue<U>& rhs) { return lhs.ptr_ == rhs.ptr_; }
        template<typename U>
        friend auto operator<=>(const CopyOnUpdSharedValue& lhs, const CopyOnUpdSharedValue<U>& rhs) { return lhs.ptr_ <=> rhs.ptr_; }

        const_pointer get() const noexcept { return std::launder(static_cast<const T*>(ptr_->data)); }
        const_pointer operator->() const noexcept { return get(); }
        const_reference operator*() const { return *get(); }

        pointer upd()
        {
            if (ptr_->owners.load(std::memory_order_acquire) == 1) {
                return std::launder(static_cast<T*>(ptr_->data));
            }
            CopyOnUpdSharedValue copy = make_cowv<T>(*get());
            swap(copy, *this);
            return std::launder(static_cast<T*>(ptr_->data));
        }

    private:
        friend struct std::hash<CopyOnUpdSharedValue>;

        template<typename U, typename... Args>
        friend CopyOnUpdSharedValue<U> make_cowv(Args&&...);

        struct ControlBlock final {
            explicit ControlBlock(void (*deleter)(ControlBlock*) noexcept, void* data) noexcept :
                deleter{deleter},
                data{data}
            {}

            std::atomic<size_t> owners = 1;
            void (*deleter)(ControlBlock*) noexcept;
            void* data;
        };

        explicit CopyOnUpdSharedValue(ControlBlock* ptr) noexcept : ptr_{ptr} {}

        template<typename... Args>
        requires std::constructible_from<T, Args&&...>
        static ControlBlock* allocate_representation(Args&&... args)
        {
            constexpr size_t allocation_alignment_size = std::max(alignof(ControlBlock), alignof(T));
            constexpr size_t allocation_size = sizeof(ControlBlock) + sizeof(T) + allocation_alignment_size - 1;

            // Allocate a memory block containing both the control block and data
            std::unique_ptr<void, void(*)(void*)> allocation{
                ::operator new(allocation_size, std::align_val_t{allocation_alignment_size}),
                [](void* p) { ::operator delete(p, std::align_val_t{allocation_alignment_size}); }
            };

            // Calculate where the data should go within the allocation (accounting for alignment)
            void* control_block_end = static_cast<std::byte*>(allocation.get()) + sizeof(ControlBlock);
            size_t available_space = allocation_size - sizeof(ControlBlock);
            void* data_start = std::align(alignof(T), sizeof(T), control_block_end, available_space);
            if (not data_start) {
                throw std::bad_alloc{};
            }

            // Generate a runtime destructor for this type
            void (*deleter)(ControlBlock*) noexcept = [](ControlBlock* cb) noexcept
            {
                std::launder(static_cast<T*>(cb->data))->~T();
                cb->~ControlBlock();
                ::operator delete(static_cast<void*>(cb), std::align_val_t{allocation_alignment_size});
            };

            // Construct representations
            new (data_start) T(std::forward<Args>(args)...);  // can throw - do it first
            static_assert(std::is_nothrow_constructible_v<ControlBlock, decltype(deleter), decltype(data_start)>);
            new (allocation.get()) ControlBlock(deleter, data_start);

            return std::launder(static_cast<ControlBlock*>(allocation.release()));
        }

        ControlBlock* ptr_;
    };

    template<typename U, typename... Args>
    CopyOnUpdSharedValue<U> make_cowv(Args&&... args)
    {
        return CopyOnUpdSharedValue<U>{CopyOnUpdSharedValue<U>::allocate_representation(std::forward<Args>(args)...)};
    }
}
