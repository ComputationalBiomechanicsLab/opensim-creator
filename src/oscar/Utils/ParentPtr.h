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
        template<
            typename U,
            typename std::enable_if_t<std::is_base_of_v<T, U>, bool> = true
        >
        explicit ParentPtr(const std::shared_ptr<U>& parent_) :
            m_Parent{parent_}
        {
            OSC_ASSERT(!m_Parent.expired() && "null or expired parent pointer given to a child");
        }

        // normal copy construction
        ParentPtr(const ParentPtr&) = default;

        // coercing copy construction: only applies when `T` can be implicitly converted to `U`
        template<
            typename U,
            typename std::enable_if_t<std::is_base_of_v<T, U>, bool> = true
        >
        ParentPtr(const ParentPtr<U>& other) :
            m_Parent{other.m_Parent}
        {}

        // normal move construction
        ParentPtr(ParentPtr&&) noexcept = default;

        // coercing move construction: only applies when `T` can be implicitly converted to `U`
        template<
            typename U,
            typename std::enable_if_t<std::is_base_of_v<T, U>, bool> = true
        >
        ParentPtr(ParentPtr<U>&& tmp) noexcept :
            m_Parent{std::move(tmp.m_Parent)}
        {
        }

        // normal copy assignment
        ParentPtr& operator=(const ParentPtr&) = default;

        // coercing copy assignment: only applies when `T` can be implicitly converted to `U`
        template<
            typename U,
            typename std::enable_if_t<std::is_base_of_v<T, U>, bool> = false
        >
        ParentPtr& operator=(const ParentPtr<U>& other)
        {
            m_Parent = other.m_Parent;
        }

        // normal move assignment
        ParentPtr& operator=(ParentPtr&&) noexcept = default;

        // coercing move assignment: only applies when `T` can be implicitly converted to `U`
        template<
            typename U,
            typename std::enable_if_t<std::is_base_of_v<T, U>, bool> = true
        >
        ParentPtr& operator=(ParentPtr<U> && tmp) noexcept
        {
            m_Parent = std::move(tmp.m_Parent);
        }

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
        template<typename TDerived, typename TBase> friend std::optional<ParentPtr<TDerived>> DynamicParentCast(const ParentPtr<TBase>&);

        std::weak_ptr<T> m_Parent;
    };

    template<
        typename TDerived,
        typename TBase
    >
    std::optional<ParentPtr<TDerived>> DynamicParentCast(const ParentPtr<TBase>& p)
    {
        const std::shared_ptr<TBase> parentSharedPtr = p.m_Parent.lock();
        OSC_ASSERT(parentSharedPtr != nullptr && "orphaned child tried to access a dead parent: this is a development error");

        if (auto parentDowncastedPtr = std::dynamic_pointer_cast<TDerived>(parentSharedPtr)) {
            return ParentPtr<TDerived>{std::move(parentDowncastedPtr)};
        }
        else {
            return std::nullopt;
        }
    }
}
