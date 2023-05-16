#pragma once

#include "src/Utils/PropertySystem/ComponentIterator.hpp"

#include <cstddef>
#include <stack>
#include <vector>

namespace osc
{
    class ComponentIterator final {
    public:
        ComponentIterator() = default;

        explicit ComponentIterator(Component& c)
        {
            m_VisitorStack.emplace(&c, -1);
        }

        Component& operator*() noexcept
        {
            return *m_VisitorStack.top().component;
        }

        Component* operator->() noexcept
        {
            return m_VisitorStack.top().component;
        }

        ComponentIterator& operator++()
        {
            StackEl& top = m_VisitorStack.top();

            ++top.pos;
            if (top.pos < static_cast<ptrdiff_t>(top.component->getNumSubcomponents()))
            {
                m_VisitorStack.emplace(&top.component->updIthSubcomponent(top.pos), -1);
            }
            else
            {
                m_VisitorStack.pop();
            }
            return *this;
        }

    private:
        friend bool operator!=(ComponentIterator const&, ComponentIterator const&) noexcept;

        struct StackEl final {
            StackEl(Component* component_, ptrdiff_t pos_) :
                component{component_},
                pos{pos_}
            {
            }

            bool operator==(StackEl const& other) const noexcept
            {
                return component == other.component && pos == other.pos;
            }

            Component* component;
            ptrdiff_t pos;
        };

        std::stack<StackEl, std::vector<StackEl>> m_VisitorStack;
    };

    bool operator!=(ComponentIterator const& a, ComponentIterator const& b) noexcept
    {
        return a.m_VisitorStack != b.m_VisitorStack;
    }
}