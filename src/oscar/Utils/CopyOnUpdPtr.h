#pragma once

#include <oscar/Utils/Concepts.h>

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace osc
{
    // `CopyOnUpdPtr` is a smart pointer that retains shared ownership of an object through
    // a pointer. Several `CopyOnUpdPtr` objects may view the same object, but it can only
    // be mutated via `upd()`, results in the following happening:
    //
    // - if there is one owner, provides mutable access to the object
    // - if there are multiple owners, copies the object and provides single-owner mutable access to the copy
    //
    // The object is destroyed and its memory deallocated when either of the following happens:
    //
    // - the last remaining `CopyOnUpdPtr` owning the object is destroyed
    // - the last remaining `CopyOnUpdPtr` owning the object is assigned another pointer via `operator=` or `reset()`
    template<typename T>
    class CopyOnUpdPtr final {
    private:
        template<typename U, typename... Args>
        friend CopyOnUpdPtr<U> make_cow(Args&&...);

        explicit CopyOnUpdPtr(std::shared_ptr<T> ptr) :
            ptr_{std::move(ptr)}
        {}
    public:
        using pointer = T*;
        using element_type = std::remove_extent_t<T>;

        const element_type* get() const noexcept
        {
            return ptr_.get();
        }

        element_type* upd()
        {
            if (ptr_.use_count() > 1) {
                ptr_ = std::make_shared<T>(*ptr_);
            }
            return ptr_.get();
        }

        const T& operator*() const noexcept
        {
            return *ptr_;
        }

        const T* operator->() const noexcept
        {
            return get();
        }

        friend void swap(CopyOnUpdPtr& a, CopyOnUpdPtr& b) noexcept
        {
            swap(a.ptr_, b.ptr_);
        }

        template<typename U>
        friend bool operator==(const CopyOnUpdPtr& lhs, const CopyOnUpdPtr<U>& rhs)
        {
            return lhs.ptr_ == rhs.ptr_;
        }

        template<typename U>
        friend auto operator<=>(const CopyOnUpdPtr& lhs, const CopyOnUpdPtr<U>& rhs)
        {
            return lhs.ptr_ <=> rhs.ptr_;
        }

    private:
        friend struct std::hash<CopyOnUpdPtr>;

        std::shared_ptr<T> ptr_;
    };

    template<typename T, typename... Args>
    CopyOnUpdPtr<T> make_cow(Args&&... args)
    {
        return CopyOnUpdPtr<T>(std::make_shared<T>(std::forward<Args>(args)...));
    }
}

template<typename T>
struct std::hash<osc::CopyOnUpdPtr<T>> final {
    size_t operator()(const osc::CopyOnUpdPtr<T>& cow) const
    {
        return std::hash<std::shared_ptr<T>>{}(cow.ptr_);
    }
};
