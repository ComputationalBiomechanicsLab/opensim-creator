#include "Class.h"

#include <oscar/DOM/PropertyInfo.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/StringHelpers.h>
#include <oscar/Utils/StringName.h>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    const StringName& validate_as_classname(const StringName& s)
    {
        if (is_valid_identifier(s)) {
            return s;
        }
        else {
            std::stringstream ss;
            ss << s << ": is not a valid class name: must an 'identifier' (i.e. start with a letter/underscore, followed by letters/numbers/underscores)";
            throw std::runtime_error{std::move(ss).str()};
        }
    }

    template<std::copy_constructible T>
    std::vector<T> concat_into_vector(std::span<const T> a, std::span<const T> b)
    {
        std::vector<T> rv;
        rv.reserve(a.size() + b.size());
        rv.insert(rv.end(), a.begin(), a.end());
        rv.insert(rv.end(), b.begin(), b.end());
        return rv;
    }

    std::unordered_map<StringName, size_t> create_index_lookup_throw_if_dupes(
        std::span<const PropertyInfo> properties)
    {
        std::unordered_map<StringName, size_t> rv;
        rv.reserve(properties.size());

        for (size_t i = 0; i < properties.size(); ++i) {
            if (not rv.try_emplace(properties[i].name(), i).second) {
                std::stringstream ss;
                ss << properties[i].name() << ": duplicate property detected: each property of an object must be unique (incl. properties from the base class)";
                throw std::runtime_error{std::move(ss).str()};
            }
        }
        return rv;
    }
}

class osc::Class::Impl final {
public:
    Impl() = default;

    Impl(
        std::string_view name,
        const Class& parent_class,
        std::span<const PropertyInfo> properties) :

        name_{validate_as_classname(StringName{name})},
        parent_class_{parent_class},
        properties_{concat_into_vector(parent_class.properties(), properties)},
        property_name_to_index_lookup_{create_index_lookup_throw_if_dupes(properties_)}
    {}

    const StringName& name() const { return name_; }
    std::optional<Class> parent_class() const { return parent_class_; }
    std::span<const PropertyInfo> properties() const { return properties_; }

    std::optional<size_t> property_index(const StringName& property_name) const
    {
        return lookup_or_nullopt(property_name_to_index_lookup_, property_name);
    }

    friend bool operator==(const Impl&, const Impl&) = default;

private:
    StringName name_{"Object"};
    std::optional<Class> parent_class_;
    std::vector<PropertyInfo> properties_;
    std::unordered_map<StringName, size_t> property_name_to_index_lookup_;
};

osc::Class::Class()
{
    static const std::shared_ptr<Impl> s_ObjectClassImpl = std::make_shared<Impl>();
    impl_ = s_ObjectClassImpl;
}

osc::Class::Class(
    std::string_view name,
    const Class& parent_class,
    std::span<const PropertyInfo> properties) :

    impl_{std::make_shared<Impl>(name, parent_class, properties)}
{}

const StringName& osc::Class::name() const { return impl_->name(); }
std::optional<Class> osc::Class::parent_class() const { return impl_->parent_class(); }
std::span<const PropertyInfo> osc::Class::properties() const { return impl_->properties(); }
std::optional<size_t> osc::Class::property_index(const StringName& property_name) const { return impl_->property_index(property_name); }

bool osc::operator==(const Class& a, const Class& b)
{
    return a.impl_ == b.impl_ or *a.impl_ == *b.impl_;
}
