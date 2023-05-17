#include "src/Utils/PropertySystem/Component.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <utility>

// helper classes that inherit from Component etc.
namespace
{
	// a blank component that isn't using the macros etc.
	class BlankComponent : public osc::Component {
	private:
		std::unique_ptr<Component> implClone() const
		{
			return std::make_unique<BlankComponent>(*this);
		}

		friend bool operator==(BlankComponent const&, BlankComponent const&) noexcept;
		float m_Value = 1.0f;
	};

	bool operator==(BlankComponent const& a, BlankComponent const& b) noexcept
	{
		return
			static_cast<osc::Component const&>(a) == static_cast<osc::Component const&>(b) &&
			a.m_Value == b.m_Value;
	}
}

TEST(Component, BlankComponentCanBeConstructed)
{
	BlankComponent c;
}

TEST(Component, BlankComponentCanBeCopyConstructed)
{
	BlankComponent a;
	BlankComponent b{a};
}

TEST(Component, BlankComponentCanBeMoveConstructed)
{
	BlankComponent a;
	BlankComponent b{std::move(a)};
}
