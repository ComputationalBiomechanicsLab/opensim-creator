#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace osc { class StringName; }
namespace osc { class PropertyInfo; }

namespace osc
{
    class Class final {
    public:
        Class();

        Class(
            std::string_view className_,
            Class const& parentClass_ = Class{},
            std::span<PropertyInfo const> propertyList_ = std::span<PropertyInfo const>{}
        );

        StringName const& getName() const;
        std::optional<Class> getParentClass() const;
        std::span<PropertyInfo const> getPropertyList() const;
        std::optional<size_t> getPropertyIndex(StringName const& propertyName) const;

        friend bool operator==(Class const&, Class const&);

    private:
        class Impl;
        std::shared_ptr<Impl const> m_Impl;
    };

    bool operator==(Class const&, Class const&);
}
