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
    const StringName& validateAsClassName(const StringName& s)
    {
        if (IsValidIdentifier(s)) {
            return s;
        }
        else {
            std::stringstream ss;
            ss << s << ": is not a valid class name: must an 'identifier' (i.e. start with a letter/underscore, followed by letters/numbers/underscores)";
            throw std::runtime_error{std::move(ss).str()};
        }
    }

    template<std::copy_constructible T>
    std::vector<T> concatIntoVector(std::span<const T> a, std::span<const T> b)
    {
        std::vector<T> rv;
        rv.reserve(a.size() + b.size());
        rv.insert(rv.end(), a.begin(), a.end());
        rv.insert(rv.end(), b.begin(), b.end());
        return rv;
    }

    std::unordered_map<StringName, size_t> createIndexLookupThrowIfDupesDetected(
        std::span<const PropertyInfo> properties)
    {
        std::unordered_map<StringName, size_t> rv;
        rv.reserve(properties.size());

        for (size_t i = 0; i < properties.size(); ++i) {
            if (!rv.try_emplace(properties[i].getName(), i).second) {
                std::stringstream ss;
                ss << properties[i].getName() << ": duplicate property detected: each property of an object must be unique (incl. properties from the base class)";
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
        std::string_view className_,
        const Class& parentClass_,
        std::span<const PropertyInfo> propertyList_) :

        m_ClassName{validateAsClassName(StringName{className_})},
        m_MaybeParentClass{parentClass_},
        m_PropertyList{concatIntoVector(parentClass_.getPropertyList(), propertyList_)},
        m_PropertyNameToPropertyListIndexMap{createIndexLookupThrowIfDupesDetected(m_PropertyList)}
    {}

    const StringName& getName() const { return m_ClassName; }
    std::optional<Class> getParentClass() const { return m_MaybeParentClass; }
    std::span<const PropertyInfo> getPropertyList() const { return m_PropertyList; }

    std::optional<size_t> getPropertyIndex(const StringName& propertyName) const
    {
        return find_or_optional(m_PropertyNameToPropertyListIndexMap, propertyName);
    }

    friend bool operator==(const Impl&, const Impl&) = default;

private:
    StringName m_ClassName{"Object"};
    std::optional<Class> m_MaybeParentClass;
    std::vector<PropertyInfo> m_PropertyList;
    std::unordered_map<StringName, size_t> m_PropertyNameToPropertyListIndexMap;
};

osc::Class::Class()
{
    static const std::shared_ptr<Impl> s_ObjectClassImpl = std::make_shared<Impl>();
    m_Impl = s_ObjectClassImpl;
}

osc::Class::Class(
    std::string_view className_,
    const Class& parentClass_,
    std::span<const PropertyInfo> propertyList_
) :
    m_Impl{std::make_shared<Impl>(className_, parentClass_, propertyList_)}
{
}

const StringName& osc::Class::getName() const
{
    return m_Impl->getName();
}

std::optional<Class> osc::Class::getParentClass() const
{
    return m_Impl->getParentClass();
}

std::span<const PropertyInfo> osc::Class::getPropertyList() const
{
    return m_Impl->getPropertyList();
}

std::optional<size_t> osc::Class::getPropertyIndex(const StringName& propertyName) const
{
    return m_Impl->getPropertyIndex(propertyName);
}

bool osc::operator==(const Class& a, const Class& b)
{
    return a.m_Impl == b.m_Impl || *a.m_Impl == *b.m_Impl;
}
