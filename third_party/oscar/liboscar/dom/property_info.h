#pragma once

#include <liboscar/utils/string_name.h>
#include <liboscar/variant/variant.h>
#include <liboscar/variant/variant_type.h>

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
