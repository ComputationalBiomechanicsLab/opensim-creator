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

        CopyOnUpdSharedValue(CopyOnUpdSharedValue&&) noexcept = delete;

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

        CopyOnUpdSharedValue& operator=(CopyOnUpdSharedValue&&) noexcept = delete;

        friend void swap(CopyOnUpdSharedValue& a, CopyOnUpdSharedValue& b) noexcept
        {
            std::swap(a.ptr_, b.ptr_);
        }

        template<typename U>
        friend bool operator==(const CopyOnUpdSharedValue& lhs, const CopyOnUpdSharedValue<U>& rhs) { return lhs.ptr_ == rhs.ptr_; }
        template<typename U>
        friend auto operator<=>(const CopyOnUpdSharedValue& lhs, const CopyOnUpdSharedValue<U>& rhs) { return lhs.ptr_ <=> rhs.ptr_; }

        const_pointer get() const noexcept { return static_cast<const T*>(ptr_->data); }
        const_pointer operator->() const noexcept { return get(); }
        const_reference operator*() const { return *get(); }

        pointer upd()
        {
            if (ptr_->owners.load(std::memory_order_acquire) == 1) {
                return static_cast<T*>(ptr_->data);
            }
            CopyOnUpdSharedValue copy{allocate_representation(*get())};
            swap(copy, *this);
            return static_cast<T*>(ptr_->data);
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
            // Calculate memory allocation layout, size, and alignment
            constexpr size_t data_offset = (sizeof(ControlBlock) + (alignof(T) - 1)) & ~(alignof(T) - 1);
            constexpr size_t allocation_size = data_offset + sizeof(T);
            constexpr std::align_val_t allocation_alignment{std::max(alignof(ControlBlock), alignof(T))};

            // Generate a type-erased and stateless destructor for `T`
            auto deleter = [](ControlBlock* cb) noexcept -> void
            {
                std::destroy_at(static_cast<T*>(cb->data));
                std::destroy_at(cb);
                ::operator delete(static_cast<void*>(cb), allocation_alignment);
            };

            // Allocate a memory block containing both the control block and data
            std::unique_ptr<void, void(*)(void*)> allocation{
                ::operator new(allocation_size, allocation_alignment),
                [](void* p) { ::operator delete(p, allocation_alignment); },
            };

            // Construct `ControlBlock` and `T` representations in the memory block
            std::byte* data_ptr = static_cast<std::byte*>(allocation.get()) + data_offset;
            new (data_ptr) T(std::forward<Args>(args)...);
            static_assert(std::is_nothrow_constructible_v<ControlBlock, decltype(deleter), decltype(data_ptr)>);
            new (allocation.get()) ControlBlock(deleter, data_ptr);

            return static_cast<ControlBlock*>(allocation.release());
        }

        ControlBlock* ptr_;
    };

    template<typename U, typename... Args>
    CopyOnUpdSharedValue<U> make_cowv(Args&&... args)
    {
        return CopyOnUpdSharedValue<U>{CopyOnUpdSharedValue<U>::allocate_representation(std::forward<Args>(args)...)};
    }
}
