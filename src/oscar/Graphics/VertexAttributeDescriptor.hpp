#pragma once

#include <oscar/Graphics/VertexAttribute.hpp>
#include <oscar/Graphics/VertexAttributeFormat.hpp>
#include <oscar/Graphics/VertexAttributeFormatDetails.hpp>

#include <cstddef>

namespace osc
{
    class VertexAttributeDescriptor {
    public:
        VertexAttributeDescriptor(
            VertexAttribute attribute_,
            VertexAttributeFormat format_) :

            m_Attribute{attribute_},
            m_Format{format_}
        {
        }

        friend bool operator==(VertexAttributeDescriptor const&, VertexAttributeDescriptor const&) = default;

        VertexAttribute attribute() const { return m_Attribute; }
        VertexAttributeFormat format() const { return m_Format; }
        size_t stride() const { return GetDetails(m_Format).stride(); }

    private:
        VertexAttribute m_Attribute;
        VertexAttributeFormat m_Format;
    };
}
