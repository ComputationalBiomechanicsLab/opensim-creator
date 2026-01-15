#pragma once

#include <liboscar/graphics/vertex_attribute.h>
#include <liboscar/graphics/vertex_attribute_format.h>

#include <cstddef>

namespace osc
{
    class VertexAttributeDescriptor {
    public:
        VertexAttributeDescriptor(
            VertexAttribute attribute,
            VertexAttributeFormat format) :

            attribute_{attribute},
            format_{format}
        {}

        friend bool operator==(const VertexAttributeDescriptor&, const VertexAttributeDescriptor&) = default;

        VertexAttribute attribute() const { return attribute_; }
        VertexAttributeFormat format() const { return format_; }
        size_t stride() const;

    private:
        VertexAttribute attribute_;
        VertexAttributeFormat format_;
    };
}
