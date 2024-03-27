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
            const Class& parentClass_ = Class{},
            std::span<const PropertyInfo> propertyList_ = std::span<const PropertyInfo>{}
        );

        const StringName& getName() const;
        std::optional<Class> getParentClass() const;
        std::span<const PropertyInfo> getPropertyList() const;
        std::optional<size_t> getPropertyIndex(const StringName& propertyName) const;

        friend bool operator==(const Class&, const Class&);

    private:
        class Impl;
        std::shared_ptr<const Impl> m_Impl;
    };

    bool operator==(const Class&, const Class&);
}
