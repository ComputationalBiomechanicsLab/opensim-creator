#include "VertexFormat.h"

#include <oscar/Graphics/VertexAttributeDescriptor.h>
#include <oscar/Shims/Cpp23/ranges.h>

#include <cstddef>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

namespace rgs = std::ranges;

osc::VertexFormat::VertexFormat(std::span<const VertexAttributeDescriptor> descriptors) :
    attribute_descriptions_{descriptors.begin(), descriptors.end()}
{
    if (attribute_descriptions_.empty()) {
        return;  // i.e. "as if" default-constructed
    }

    if (attribute_descriptions_.front().attribute() != VertexAttribute::Position) {
        throw std::runtime_error{"Invalid `VertexFormat`, a `VertexFormat` should always contain at least `VertexAttribute::Position`"};
    }

    for (auto it = attribute_descriptions_.begin(), end = attribute_descriptions_.end(); it != end; ++it) {
        if (cpp23::contains(it+1, end, it->attribute(), &VertexAttributeDescriptor::attribute)) {
            throw std::runtime_error{"Duplicate `VertexAttribute`s were passed to a `VertexFormat` each `VertexAttribute` must be unique"};
        }
    }

    stride_ = calc_stride();
}

void osc::VertexFormat::insert(const VertexAttributeDescriptor& desc)
{
    if (attribute_descriptions_.empty() and desc.attribute() != VertexAttribute::Position) {
        return;
    }

    const auto it = rgs::find(attribute_descriptions_, desc.attribute(), &VertexAttributeDescriptor::attribute);
    if (it != attribute_descriptions_.end()) {
        *it = desc;
    }
    else {
        attribute_descriptions_.push_back(desc);
    }

    stride_ = calc_stride();
}

void osc::VertexFormat::erase(VertexAttribute attribute)
{
    if (attribute == VertexAttribute::Position) {
        // clearing Position clears the position (primary) data and all other attributes
        clear();
        return;
    }

    const auto it = rgs::find(attribute_descriptions_, attribute, &VertexAttributeDescriptor::attribute);
    if (it != attribute_descriptions_.end()) {
        attribute_descriptions_.erase(it);
        stride_ = calc_stride();
    }
}

size_t osc::VertexFormat::calc_stride() const
{
    size_t rv = 0;
    for (const auto& descriptor : attribute_descriptions_) {
        rv += descriptor.stride();
    }
    return rv;
}
