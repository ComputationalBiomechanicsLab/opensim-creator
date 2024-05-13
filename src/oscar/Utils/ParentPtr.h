#pragma once

#include <oscar/Utils/Assertions.h>

#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace osc
{
    // parent pointer
    //
    // a non-nullable, non-owning, pointer to a parent element in a hierarchy with
    // runtime lifetime checks
    template<typename T>
    class ParentPtr final {
    public:

        // construct from a non-null shared ptr
        template<std::derived_from<T> U>
        explicit ParentPtr(const std::shared_ptr<U>& parent) :
            ptr_{parent}
        {
            OSC_ASSERT(!ptr_.expired() && "null or expired parent pointer given to a child");
        }

        // normal copy construction
        ParentPtr(const ParentPtr&) = default;

        // coercing copy construction: only applies when `T` can be implicitly converted to `U`
        template<std::derived_from<T> U>
        ParentPtr(const ParentPtr<U>& other) :
            ptr_{other.ptr_}
        {}

        // normal move construction
        ParentPtr(ParentPtr&&) noexcept = default;

        // coercing move construction: only applies when `T` can be implicitly converted to `U`
        template<std::derived_from<T> U>
        ParentPtr(ParentPtr<U>&& tmp) noexcept :
            ptr_{std::move(tmp.ptr_)}
        {}

        // normal copy assignment
        ParentPtr& operator=(const ParentPtr&) = default;

        // coercing copy assignment: only applies when `T` can be implicitly converted to `U`
        template<std::derived_from<T> U>
        ParentPtr& operator=(const ParentPtr<U>& other)
        {
            ptr_ = other.ptr_;
        }

        // normal move assignment
        ParentPtr& operator=(ParentPtr&&) noexcept = default;

        // coercing move assignment: only applies when `T` can be implicitly converted to `U`
        template<std::derived_from<T> U>
        ParentPtr& operator=(ParentPtr<U> && tmp) noexcept
        {
            ptr_ = std::move(tmp.ptr_);
        }

        ~ParentPtr() noexcept = default;

        T* operator->() const
        {
            OSC_ASSERT(!ptr_.expired() && "orphaned child tried to access a dead parent: this is a development error");
            return ptr_.lock().get();
        }

        T& operator*() const
        {
            OSC_ASSERT(!ptr_.expired() && "orphaned child tried to access a dead parent: this is a development error");
            return *ptr_.lock();
        }

        friend void swap(ParentPtr& a, ParentPtr& b) noexcept
        {
            std::swap(a.ptr_, b.ptr_);
        }

    private:
        // self-friend-class for the coercing constructor
        template<typename> friend class ParentPtr;

        // friend function, for downcasting
        template<typename TDerived, typename TBase>
        friend std::optional<ParentPtr<TDerived>> dynamic_parent_cast(const ParentPtr<TBase>&);

        std::weak_ptr<T> ptr_;
    };

    template<typename TDerived, typename TBase>
    std::optional<ParentPtr<TDerived>> dynamic_parent_cast(const ParentPtr<TBase>& p)
    {
        static_assert(std::derived_from<TDerived, TBase>);

        const std::shared_ptr<TBase> parent_shared_ptr = p.ptr_.lock();
        OSC_ASSERT_ALWAYS(parent_shared_ptr != nullptr && "orphaned child tried to access a dead parent: this is a development error");

        if (auto parent_downcasted_ptr = std::dynamic_pointer_cast<TDerived>(parent_shared_ptr)) {
            return ParentPtr<TDerived>{std::move(parent_downcasted_ptr)};
        }
        else {
            return std::nullopt;
        }
    }
}
