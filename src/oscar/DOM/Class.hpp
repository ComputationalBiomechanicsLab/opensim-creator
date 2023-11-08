#pragma once

#include <oscar/Utils/StringName.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>

namespace osc { class PropertyInfo; }

namespace osc
{
	class Class final {
	public:
		Class(
			StringName const& className_,
			std::optional<Class> maybeParentClass_,
			std::span<PropertyInfo const> propertyList_
		);

		StringName const& getName() const;
		std::optional<Class> tryGetParentClass() const;
		std::span<PropertyInfo const> getPropertyList() const;
		std::optional<size_t> indexOfPropertyInPropertyList(StringName const& propertyName) const;

	private:
		class Impl;
		std::shared_ptr<Impl const> m_Impl;
	};
}