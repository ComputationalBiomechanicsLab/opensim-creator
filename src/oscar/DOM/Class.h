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

        explicit Class(
            std::string_view name,
            const Class& parent_class = Class{},
            std::span<const PropertyInfo> properties = std::span<const PropertyInfo>{}
        );

        const StringName& name() const;
        std::optional<Class> parent_class() const;
        std::span<const PropertyInfo> properties() const;
        std::optional<size_t> property_index(const StringName& property_name) const;

        friend bool operator==(const Class&, const Class&);

    private:
        class Impl;
        std::shared_ptr<const Impl> impl_;
    };

    bool operator==(const Class&, const Class&);
}
