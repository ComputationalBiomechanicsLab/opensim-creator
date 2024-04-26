#include "VertexFormat.h"

#include <oscar/Graphics/VertexAttributeDescriptor.h>
#include <oscar/Utils/Algorithms.h>

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <ranges>
#include <stdexcept>
#include <vector>

namespace rgs = std::ranges;

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
        if (std::ranges::count_if(m_AttributeDescriptions, hasAttr) > 1)
        {
            throw std::runtime_error{"Duplicate attributes passed to VertexFormat: each VertexAttribute should be unique"};
        }
    }
    stride_ = calc_stride();
}

void osc::VertexFormat::insert(VertexAttributeDescriptor const& desc)
{
    if (m_AttributeDescriptions.empty() && desc.attribute() != VertexAttribute::Position)
    {
        return;
    }

    auto const it = rgs::lower_bound(
        m_AttributeDescriptions,
        desc.attribute(),
        rgs::less{},
        &VertexAttributeDescriptor::attribute
    );

    if (it != m_AttributeDescriptions.end() && it->attribute() == desc.attribute())
    {
        *it = desc;
    }
    else
    {
        m_AttributeDescriptions.insert(it, desc);
    }
    stride_ = calc_stride();
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
    stride_ = calc_stride();
}

size_t osc::VertexFormat::calc_stride() const
{
    size_t rv = 0;
    for (auto const& desc : m_AttributeDescriptions)
    {
        rv += desc.stride();
    }
    return rv;
}
