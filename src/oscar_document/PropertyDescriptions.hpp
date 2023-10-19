#pragma once

#include <oscar_document/PropertyDescription.hpp>

#include <cstddef>
#include <vector>

namespace osc::doc { class PropertyDescription; }

namespace osc::doc
{
    class PropertyDescriptions final {
    public:
        auto begin() const
        {
            return m_Descriptions.begin();
        }

        auto end() const
        {
            return m_Descriptions.end();
        }

        size_t size() const
        {
            return m_Descriptions.size();
        }

        PropertyDescription const& at(size_t i) const
        {
            return m_Descriptions.at(i);
        }

        void append(PropertyDescription const&);

        friend bool operator==(PropertyDescriptions const& a, PropertyDescriptions const& b)
        {
            return a.m_Descriptions == b.m_Descriptions;
        }
        friend bool operator!=(PropertyDescriptions const& a, PropertyDescriptions const& b)
        {
            return !(a == b);
        }
    private:
        std::vector<PropertyDescription> m_Descriptions;
    };
}
