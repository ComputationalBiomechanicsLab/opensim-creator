#pragma once

#include <oscar/Maths/AnalyticPlane.h>

#include <cstddef>

namespace osc
{
    // represents the six outward-pointing clipping planes of a frustum
    struct FrustumPlanes final {
        using value_type = AnalyticPlane;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = AnalyticPlane&;
        using const_reference = const AnalyticPlane&;
        using pointer = AnalyticPlane*;
        using const_pointer = const AnalyticPlane*;
        using iterator = AnalyticPlane*;
        using const_iterator = const AnalyticPlane*;

        friend constexpr bool operator==(const FrustumPlanes&, const FrustumPlanes&) = default;

        constexpr size_t size() const { return 6; }
        constexpr pointer data() { return &p0; }
        constexpr const_pointer data() const { return &p0; }
        constexpr iterator begin() { return data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr iterator end() { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr reference operator[](size_t pos) { return begin()[pos]; }
        constexpr const_reference operator[](size_t pos) const { return begin()[pos]; }

        AnalyticPlane p0 = {.distance = 0.0f, .normal = { 0.0f,  0.0f, -1.0f}};
        AnalyticPlane p1 = {.distance = 1.0f, .normal = { 0.0f,  0.0f,  1.0f}};
        AnalyticPlane p2 = {.distance = 1.0f, .normal = { 1.0f,  0.0f,  0.0f}};
        AnalyticPlane p3 = {.distance = 1.0f, .normal = {-1.0f,  0.0f,  0.0f}};
        AnalyticPlane p4 = {.distance = 1.0f, .normal = { 0.0f,  1.0f,  0.0f}};
        AnalyticPlane p5 = {.distance = 1.0f, .normal = { 0.0f, -1.0f,  0.0f}};
    };
}
