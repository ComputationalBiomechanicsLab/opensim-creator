#pragma once

#include "oscar/Utils/PropertySystem/ComponentMemberOffset.hpp"
#include "oscar/Utils/ClonePtr.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace osc { class AbstractProperty; }
namespace osc { class AbstractSocket; }
namespace osc { class ComponentPath; }

namespace osc
{
    // COMPONENT
    //
    // - a named object
    // - that may have a parent
    // - and may own:
    //
    //   - properties (simple values)
    //   - sockets (graph edges)
    //   - components (children)
    class Component {
    protected:
        Component();
        Component(Component const&);
        Component(Component&&) noexcept;
        Component& operator=(Component const&) = delete;
        Component& operator=(Component&&) noexcept = delete;
    public:
        virtual ~Component() noexcept = default;

        std::unique_ptr<Component> clone() const { return implClone(); }

        Component const* tryGetParent() const;
        Component* tryUpdParent();

        CStringView getName() const;
        void setName(std::string_view);

        size_t getNumProperties() const;
        AbstractProperty const& getIthProperty(ptrdiff_t) const;
        AbstractProperty& updIthProperty(ptrdiff_t);

        size_t getNumSockets() const;
        AbstractSocket const& getIthSocket(ptrdiff_t) const;
        AbstractSocket& updIthSocket(ptrdiff_t);

        size_t getNumSubcomponents() const;
        Component const& getIthSubcomponent(ptrdiff_t) const;
        Component& updIthSubcomponent(ptrdiff_t);
        Component const* tryGetSubcomponentByName(std::string_view) const;

    private:
        friend class ComponentList;
        friend void RegisterSocketInParent(
            Component& parent,
            AbstractSocket const&,
            ComponentMemberOffset
        );
        friend void RegisterPropertyInParent(
            Component& parent,
            AbstractProperty const&,
            ComponentMemberOffset
        );
        friend Component const* TryFindComponent(
            Component const&,
            ComponentPath const&
        );

        virtual std::unique_ptr<Component> implClone() const = 0;

        Component* m_Parent;
        std::string m_Name;
        std::vector<ComponentMemberOffset> m_DeclarationOrderedPropertyOffsets;
        std::vector<ComponentMemberOffset> m_DeclarationOrderedSocketOffsets;
        std::vector<ClonePtr<Component>> m_LexographicallyOrderedSubcomponents;
    };

    void RegisterPropertyInParent(Component& parent, AbstractProperty const&, ComponentMemberOffset);
    void RegisterSocketInParent(Component& parent, AbstractSocket const&, ComponentMemberOffset);
    Component const& GetRoot(Component const&);
    Component const* TryFindComponent(Component const&, ComponentPath const&);
    Component* TryFindComponentMut(Component&, ComponentPath const&);
}
