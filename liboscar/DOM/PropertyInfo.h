#pragma once

#include <liboscar/Utils/StringName.h>
#include <liboscar/Variant/Variant.h>
#include <liboscar/Variant/VariantType.h>

#include <string_view>
#include <utility>

namespace osc
{
    class PropertyInfo final {
    public:
        PropertyInfo() = default;

        explicit PropertyInfo(
            std::string_view name,
            Variant default_value) :

            PropertyInfo{StringName{name}, std::move(default_value)}
        {}

        explicit PropertyInfo(
            StringName name,
            Variant default_value
        );

        const StringName& name() const { return name_; }

        VariantType type() const { return default_value_.type(); }

        const Variant& default_value() const { return default_value_; }

        friend bool operator==(const PropertyInfo&, const PropertyInfo&) = default;

    private:
        StringName name_;
        Variant default_value_;
    };
}
