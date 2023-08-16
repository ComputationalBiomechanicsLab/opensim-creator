#pragma once

#include "oscar/Utils/Assertions.hpp"

#include <memory>
#include <optional>
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
        template<typename U>
        explicit ParentPtr(std::shared_ptr<U> const& parent_) :
            m_Parent{parent_}
        {
            OSC_ASSERT(!m_Parent.expired() && "null or expired parent pointer given to a child");
        }

        // coercing constructor that applies when `T` can be implicitly converted to `U`
        template<typename U>
        ParentPtr(ParentPtr<U> const& other) noexcept :
            m_Parent{other.m_Parent}
        {
        }

        ParentPtr(ParentPtr&&) noexcept = default;
        ParentPtr(ParentPtr const&) noexcept = default;
        ParentPtr& operator=(ParentPtr&&) noexcept = default;
        ParentPtr& operator=(ParentPtr const&) noexcept = default;
        ~ParentPtr() noexcept = default;

        T* operator->() const
        {
            OSC_ASSERT(!m_Parent.expired() && "orphaned child tried to access a dead parent: this is a development error");
            return m_Parent.lock().get();
        }

        T& operator*() const
        {
            OSC_ASSERT(!m_Parent.expired() && "orphaned child tried to access a dead parent: this is a development error");
            return *m_Parent.lock();
        }

        friend void swap(ParentPtr& a, ParentPtr& b) noexcept
        {
            std::swap(a.m_Parent, b.m_Parent);
        }

    private:
        // self-friend-class for the coercing constructor
        template<typename> friend class ParentPtr;

        // friend function, for downcasting
        template<typename TDerived, typename TBase> friend std::optional<ParentPtr<TDerived>> DynamicParentCast(ParentPtr<TBase> const&);

        std::weak_ptr<T> m_Parent;
    };

    template<typename TDerived, typename TBase>
    std::optional<ParentPtr<TDerived>> DynamicParentCast(ParentPtr<TBase> const& p)
    {
        std::shared_ptr<TBase> const parentSharedPtr = p.m_Parent.lock();
        OSC_ASSERT(parentSharedPtr != nullptr && "orphaned child tried to access a dead parent: this is a development error");

        if (auto parentDowncastedPtr = std::dynamic_pointer_cast<TDerived>(parentSharedPtr))
        {
            return ParentPtr<TDerived>{std::move(parentDowncastedPtr)};
        }
        else
        {
            return std::nullopt;
        }
    }
}
