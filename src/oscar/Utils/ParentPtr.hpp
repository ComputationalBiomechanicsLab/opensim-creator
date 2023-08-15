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
	// (debug) existence checks for the parent
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
		ParentPtr(ParentPtr<U> const& other) : m_Parent{other.m_Parent}
		{
		}

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

	private:
		// self-friend-class for the coercing constructor
		template<typename> friend class ParentPtr;

		// friend function, for downcasting
		template<typename U, typename T> friend std::optional<ParentPtr<U>> DynamicParentCast(ParentPtr<T> const&);

		std::weak_ptr<T> m_Parent;
	};

	template<typename U, typename T>
	std::optional<ParentPtr<U>> DynamicParentCast(ParentPtr<T> const& p)
	{
		std::shared_ptr<T> const parentSharedPtr = p.m_Parent.lock();
		OSC_ASSERT(parentSharedPtr != nullptr && "orphaned child tried to access a dead parent: this is a development error");
		
		if (std::shared_ptr<U> parentDowncastedPtr = std::dynamic_pointer_cast<U>(parentSharedPtr))
		{
			return ParentPtr<U>{std::move(parentDowncastedPtr)};
		}
		else
		{
			return std::nullopt;
		}
	}
}