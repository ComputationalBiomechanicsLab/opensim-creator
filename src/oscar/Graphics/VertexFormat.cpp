#include "VertexFormat.hpp"

#include <oscar/Graphics/VertexAttributeDescriptor.hpp>

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <vector>

osc::VertexFormat::VertexFormat(std::initializer_list<VertexAttributeDescriptor> attrs) :
    m_AttributeDescriptions{attrs.begin(), attrs.end()}
{
    if (m_AttributeDescriptions.empty())
    {
        return;
    }

    if (m_AttributeDescriptions.front().attribute() != VertexAttribute::Position)
    {
        throw std::runtime_error{"Invalid first VertexAttributeDescriptor: the 'Position' field must come first"};
    }

    for (auto const& desc : m_AttributeDescriptions)
    {
        auto const hasAttr = [attr = desc.attribute()](auto const& d)
        {
            return d.attribute() == attr;
        };
        if (std::count_if(m_AttributeDescriptions.begin(), m_AttributeDescriptions.end(), hasAttr) > 1)
        {
            throw std::runtime_error{"Duplicate attributes passed to VertexFormat: each VertexAttribute should be unique"};
        }
    }
}

void osc::VertexFormat::insert(VertexAttributeDescriptor const& desc)
{
    if (m_AttributeDescriptions.empty() && desc.attribute() != VertexAttribute::Position)
    {
        return;
    }

    auto const comp = [](VertexAttributeDescriptor const& a, VertexAttributeDescriptor const& b)
    {
        return a.attribute() < b.attribute();
    };
    auto const it = std::lower_bound(
        m_AttributeDescriptions.begin(),
        m_AttributeDescriptions.end(),
        desc,
        comp
    );

    if (it != m_AttributeDescriptions.end() && it->attribute() == desc.attribute())
    {
        *it = desc;
    }
    else
    {
        m_AttributeDescriptions.insert(it, desc);
    }
}

void osc::VertexFormat::erase(VertexAttribute attr)
{
    if (attr == VertexAttribute::Position)
    {
        // clearing Position clears the position (primary) data and all other attributes
        clear();
        return;
    }

    std::erase_if(m_AttributeDescriptions, [attr](auto const& desc) { return desc.attribute() == attr; });
}
