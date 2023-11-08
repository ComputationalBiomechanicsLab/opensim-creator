#pragma once

#include <oscar/DOM/Class.hpp>
#include <oscar/Utils/StringName.hpp>
#include <oscar/Utils/Variant.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace osc
{
    class Object {
    protected:
        explicit Object(Class const&);
        Object(Object const&) = default;
        Object(Object&&) noexcept = default;
        Object& operator=(Object const&) = default;
        Object& operator=(Object&&) noexcept = default;

        bool trySetValueInPropertyTable(StringName const& propertyName, Variant const& newPropertyValue);
    public:
        static Class const& getClassStatic();

        virtual ~Object() noexcept = default;

        std::string toString() const
        {
            return implToString();
        }

        std::unique_ptr<Object> clone() const
        {
            return implClone();
        }

        Class const& getClass() const
        {
            return m_Class;
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
        virtual bool implCustomSetter(StringName const& propertyName, Variant const& newPropertyValue);

        Class m_Class;
        std::vector<Variant> m_PropertyValues;
    };

    inline std::string to_string(Object const& o)
    {
        return o.toString();
    }
}
