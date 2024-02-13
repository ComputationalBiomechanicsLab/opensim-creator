#include "Class.h"

#include <oscar/DOM/PropertyInfo.h>
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
    StringName const& ValidateAsClassName(StringName const& s)
    {
        if (IsValidIdentifier(s))
        {
            return s;
        }
        else
        {
            std::stringstream ss;
            ss << s << ": is not a valid class name: must an 'identifier' (i.e. start with a letter/underscore, followed by letters/numbers/underscores)";
            throw std::runtime_error{std::move(ss).str()};
        }
    }

    template<std::copy_constructible T>
    std::vector<T> ConcatIntoVector(std::span<T const> a, std::span<T const> b)
    {
        std::vector<T> rv;
        rv.reserve(a.size() + b.size());
        rv.insert(rv.end(), a.begin(), a.end());
        rv.insert(rv.end(), b.begin(), b.end());
        return rv;
    }

    std::unordered_map<StringName, size_t> CreateIndexLookupThrowIfDuplicatesDetected(std::span<PropertyInfo const> properties)
    {
        std::unordered_map<StringName, size_t> rv;
        rv.reserve(properties.size());
        for (size_t i = 0; i < properties.size(); ++i)
        {
            auto const [it, inserted] = rv.try_emplace(properties[i].getName(), i);
            if (!inserted)
            {
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
        Class const& parentClass_,
        std::span<PropertyInfo const> propertyList_
    ) :
        m_ClassName{ValidateAsClassName(StringName{className_})},
        m_MaybeParentClass{parentClass_},
        m_PropertyList{ConcatIntoVector(parentClass_.getPropertyList(), propertyList_)},
        m_PropertyNameToPropertyListIndexMap{CreateIndexLookupThrowIfDuplicatesDetected(m_PropertyList)}
    {
    }

    StringName const& getName() const
    {
        return m_ClassName;
    }

    std::optional<Class> getParentClass() const
    {
        return m_MaybeParentClass;
    }

    std::span<PropertyInfo const> getPropertyList() const
    {
        return m_PropertyList;
    }

    std::optional<size_t> getPropertyIndex(StringName const& propertyName) const
    {
        if (auto const it = m_PropertyNameToPropertyListIndexMap.find(propertyName); it != m_PropertyNameToPropertyListIndexMap.end())
        {
            return it->second;
        }
        else
        {
            return std::nullopt;
        }
    }

    friend bool operator==(Impl const&, Impl const&) = default;

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
    Class const& parentClass_,
    std::span<PropertyInfo const> propertyList_
) :
    m_Impl{std::make_shared<Impl>(className_, parentClass_, propertyList_)}
{
}

StringName const& osc::Class::getName() const
{
    return m_Impl->getName();
}

std::optional<Class> osc::Class::getParentClass() const
{
    return m_Impl->getParentClass();
}

std::span<PropertyInfo const> osc::Class::getPropertyList() const
{
    return m_Impl->getPropertyList();
}

std::optional<size_t> osc::Class::getPropertyIndex(StringName const& propertyName) const
{
    return m_Impl->getPropertyIndex(propertyName);
}

bool osc::operator==(Class const& a, Class const& b)
{
    return a.m_Impl == b.m_Impl || *a.m_Impl == *b.m_Impl;
}
