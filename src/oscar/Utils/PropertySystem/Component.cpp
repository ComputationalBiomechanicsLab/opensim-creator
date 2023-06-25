#include "Component.hpp"

#include "oscar/Utils/PropertySystem/AbstractSocket.hpp"
#include "oscar/Utils/PropertySystem/ComponentIterator.hpp"
#include "oscar/Utils/PropertySystem/ComponentList.hpp"
#include "oscar/Utils/PropertySystem/ComponentMemberOffset.hpp"
#include "oscar/Utils/PropertySystem/ComponentPath.hpp"
#include "oscar/Utils/ClonePtr.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

osc::Component::Component() :
    m_Parent{nullptr},
    m_Name{},
    m_DeclarationOrderedPropertyOffsets{},
    m_DeclarationOrderedSocketOffsets{},
    m_LexographicallyOrderedSubcomponents{}
{
}

osc::Component::Component(Component const& src) :
    m_Parent{nullptr},
    m_Name{src.m_Name},
    m_DeclarationOrderedPropertyOffsets{src.m_DeclarationOrderedPropertyOffsets},
    m_DeclarationOrderedSocketOffsets{src.m_DeclarationOrderedSocketOffsets},
    m_LexographicallyOrderedSubcomponents{src.m_LexographicallyOrderedSubcomponents}
{
    for (ClonePtr<Component> const& subcomponent : m_LexographicallyOrderedSubcomponents)
    {
        subcomponent->m_Parent = this;
    }
}

osc::Component::Component(Component&& tmp) noexcept :
    m_Parent{nullptr},
    m_Name{std::move(tmp.m_Name)},
    m_DeclarationOrderedPropertyOffsets{std::move(tmp.m_DeclarationOrderedPropertyOffsets)},
    m_DeclarationOrderedSocketOffsets{std::move(tmp.m_DeclarationOrderedSocketOffsets)},
    m_LexographicallyOrderedSubcomponents{std::move(tmp.m_LexographicallyOrderedSubcomponents)}
{
    for (ClonePtr<Component> const& subcomponent : m_LexographicallyOrderedSubcomponents)
    {
        subcomponent->m_Parent = this;
    }
}

osc::Component const* osc::Component::tryGetParent() const
{
    return m_Parent;
}

osc::Component* osc::Component::tryUpdParent()
{
    return m_Parent;
}

osc::CStringView osc::Component::getName() const
{
    return m_Name;
}

void osc::Component::setName(std::string_view)
{
    if (!tryGetParent() || !dynamic_cast<ComponentList const*>(tryGetParent()))
    {
        return;  // can only rename a component that's within a component list
    }

    for (auto it = ComponentIterator{*this}; it != ComponentIterator{}; ++it)
    {
        for (size_t i = 0, nSockets = it->getNumSockets(); i < nSockets; ++i)
        {
            std::string_view const path = it->getIthSocket(i).getConnecteePath();
            auto elBegin = path.begin();
            while (elBegin != path.end())
            {
                auto elEnd = std::find(elBegin, path.end(), ComponentPath::delimiter());
                std::string_view const el(&*elBegin, elEnd - elBegin);
                // TODO: rename path fragments etc.
            }
        }
    }
}

size_t osc::Component::getNumProperties() const
{
    return m_DeclarationOrderedPropertyOffsets.size();
}

osc::AbstractProperty const& osc::Component::getIthProperty(ptrdiff_t i) const
{
    ComponentMemberOffset const offset = m_DeclarationOrderedPropertyOffsets.at(i);
    char const* memoryLocation = reinterpret_cast<char const*>(this) + offset;
    return *reinterpret_cast<AbstractProperty const*>(memoryLocation);
}

osc::AbstractProperty& osc::Component::updIthProperty(ptrdiff_t i)
{
    ComponentMemberOffset const offset = m_DeclarationOrderedPropertyOffsets.at(i);
    char* memoryLocation = reinterpret_cast<char*>(this) + offset;
    return *reinterpret_cast<AbstractProperty*>(memoryLocation);
}

size_t osc::Component::getNumSockets() const
{
    return m_DeclarationOrderedSocketOffsets.size();
}

osc::AbstractSocket const& osc::Component::getIthSocket(ptrdiff_t i) const
{
    ComponentMemberOffset const offset = m_DeclarationOrderedSocketOffsets.at(i);
    char const* memoryLocation = reinterpret_cast<char const*>(this) + offset;
    return *reinterpret_cast<AbstractSocket const*>(memoryLocation);
}

osc::AbstractSocket& osc::Component::updIthSocket(ptrdiff_t i)
{
    ComponentMemberOffset const offset = m_DeclarationOrderedSocketOffsets.at(i);
    char* memoryLocation = reinterpret_cast<char*>(this) + offset;
    return *reinterpret_cast<AbstractSocket*>(memoryLocation);
}

size_t osc::Component::getNumSubcomponents() const
{
    return m_LexographicallyOrderedSubcomponents.size();
}

osc::Component const& osc::Component::getIthSubcomponent(ptrdiff_t i) const
{
    return *m_LexographicallyOrderedSubcomponents.at(i);
}

osc::Component& osc::Component::updIthSubcomponent(ptrdiff_t i)
{
    return *m_LexographicallyOrderedSubcomponents.at(i);
}

osc::Component const* osc::Component::tryGetSubcomponentByName(std::string_view name) const
{
    auto const begin = m_LexographicallyOrderedSubcomponents.begin();
    auto const end = m_LexographicallyOrderedSubcomponents.end();
    auto const lexographicallyLowerThanName = [name](osc::ClonePtr<Component> const& c)
    {
        return c->getName() < name;
    };
    auto const it = std::partition_point(begin, end, lexographicallyLowerThanName);
    return (it != end && (*it)->getName() == name) ? it->get() : nullptr;
}

osc::Component const& osc::GetRoot(Component const& component)
{
    Component const* rv = &component;
    while (Component const* parent = rv->tryGetParent())
    {
        rv = parent;
    }
    return *rv;
}

void osc::RegisterSocketInParent(
    Component& parent,
    AbstractSocket const&,
    ComponentMemberOffset offset)
{
    parent.m_DeclarationOrderedSocketOffsets.push_back(offset);
}

osc::Component const* osc::TryFindComponent(
    Component const& component,
    ComponentPath const& path)
{
    std::string_view const pathSV = path;

    Component const* c = IsAbsolute(path) ? &GetRoot(component) : &component;
    auto elBegin = pathSV.begin();
    while (c && elBegin != pathSV.end())
    {
        auto const elEnd = std::find(elBegin, pathSV.end(), ComponentPath::delimiter());
        std::string_view const el(&*elBegin, elEnd - elBegin);

        c = c->tryGetSubcomponentByName(el);
        elBegin = elEnd;
    }

    return c;
}

osc::Component* osc::TryFindComponentMut(
    Component& component,
    ComponentPath const& path)
{
    return const_cast<Component*>(TryFindComponent(component, path));
}

void osc::RegisterPropertyInParent(
    Component& parent,
    AbstractProperty const&,
    ComponentMemberOffset offset)
{
    parent.m_DeclarationOrderedPropertyOffsets.push_back(offset);
}