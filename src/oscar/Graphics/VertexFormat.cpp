#include "VertexFormat.h"

#include <oscar/Graphics/VertexAttributeDescriptor.h>
#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/Utils/Algorithms.h>

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <vector>

namespace rgs = std::ranges;

osc::VertexFormat::VertexFormat(std::initializer_list<VertexAttributeDescriptor> attrs) :
    attribute_descriptions_{attrs.begin(), attrs.end()}
{
    if (attribute_descriptions_.empty()) {
        return;  // i.e. "as if" default-constructed
    }

    // always sort by `attribute` (required for `adjacent_find`, `lower_bound`)
    rgs::sort(attribute_descriptions_, rgs::less{}, &VertexAttributeDescriptor::attribute);

    if (attribute_descriptions_.front().attribute() != VertexAttribute::Position) {
        throw std::runtime_error{"Invalid `VertexFormat`, a `VertexFormat` should always contain at least `VertexAttribute::Position`"};
    }

    const auto it = rgs::adjacent_find(attribute_descriptions_, rgs::equal_to{}, &VertexAttributeDescriptor::attribute);
    if (it != attribute_descriptions_.end()) {
        throw std::runtime_error{"Duplicate `VertexAttribute`s were passed to a `VertexFormat` each `VertexAttribute` must be unique"};
    }

    stride_ = calc_stride();
}

void osc::VertexFormat::insert(VertexAttributeDescriptor const& desc)
{
    if (attribute_descriptions_.empty() and desc.attribute() != VertexAttribute::Position) {
        return;
    }

    auto const it = rgs::lower_bound(
        attribute_descriptions_,
        desc.attribute(),
        rgs::less{},
        &VertexAttributeDescriptor::attribute
    );

    if (it != attribute_descriptions_.end() and it->attribute() == desc.attribute()) {
        *it = desc;
    }
    else {
        attribute_descriptions_.insert(it, desc);
    }

    stride_ = calc_stride();
}

void osc::VertexFormat::erase(VertexAttribute attr)
{
    if (attr == VertexAttribute::Position) {
        // clearing Position clears the position (primary) data and all other attributes
        clear();
        return;
    }

    std::erase_if(attribute_descriptions_, [attr](auto const& desc) { return desc.attribute() == attr; });
    stride_ = calc_stride();
}

size_t osc::VertexFormat::calc_stride() const
{
    size_t rv = 0;
    for (auto const& desc : attribute_descriptions_) {
        rv += desc.stride();
    }
    return rv;
}
