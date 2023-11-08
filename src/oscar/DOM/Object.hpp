#pragma once

#include <oscar/DOM/PropertyTable.hpp>
#include <oscar/Utils/StringName.hpp>
#include <oscar/Utils/Variant.hpp>
#include <oscar/Utils/VariantType.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>

namespace osc { class PropertyDescription; }

namespace osc
{
    class SetPropertyStrategy final {
    public:
        static SetPropertyStrategy Default()
        {
            return SetPropertyStrategy{DefaultBehavior{}};
        }
        static SetPropertyStrategy CoerceValueTo(Variant val)
        {
            return SetPropertyStrategy{CoercedValue{std::move(val)}};
        }
        static SetPropertyStrategy NewValueIsInvalid(std::string why)
        {
            return SetPropertyStrategy{InvalidValueErrorMessage{std::move(why)}};
        }
    private:
        template<typename TVariantMember>
        explicit SetPropertyStrategy(TVariantMember&& value_) : m_Data{std::forward<TVariantMember>(value_)} {}

        friend class Object;
        struct DefaultBehavior final {};
        struct CoercedValue final { Variant payload; };
        struct InvalidValueErrorMessage final { std::string payload; };
        std::variant<DefaultBehavior, CoercedValue, InvalidValueErrorMessage> m_Data;
    };

    class Object {
    protected:
        Object() = default;
        explicit Object(std::span<PropertyDescription const>);
        Object(Object const&) = default;
        Object(Object&&) noexcept = default;
        Object& operator=(Object const&) = default;
        Object& operator=(Object&&) noexcept = default;
    public:
        virtual ~Object() noexcept = default;

        std::string toString() const
        {
            return implToString();
        }

        std::unique_ptr<Object> clone() const
        {
            return implClone();
        }

        size_t getNumProperties() const;
        StringName const& getPropertyName(size_t propertyIndex) const;

        std::optional<size_t> getPropertyIndex(StringName const& propertyName) const;
        Variant const* tryGetPropertyDefaultValue(StringName const& propertyName) const;
        Variant const& getPropertyDefaultValue(StringName const& propertyName) const;
        Variant const* tryGetPropertyValue(StringName const& propertyName) const;
        Variant const& getPropertyValue(StringName const& propertyName) const;
        bool trySetPropertyValue(StringName const& propertyName, Variant const& newPropertyValue);
        void setPropertyValue(StringName const& propertyName, Variant const& newPropertyValue);

    private:
        virtual std::string implToString() const;
        virtual std::unique_ptr<Object> implClone() const = 0;
        virtual SetPropertyStrategy implSetPropertyStrategy(StringName const& propertyName, Variant const& newPropertyValue);

        PropertyTable m_PropertyTable;
    };

    inline std::string to_string(Object const& o)
    {
        return o.toString();
    }
}
