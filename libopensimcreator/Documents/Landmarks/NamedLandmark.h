#pragma once

#include <libopensimcreator/Documents/Landmarks/Landmark.h>

#include <liboscar/maths/Vector3.h>

#include <string>

namespace osc::lm
{
    struct NamedLandmark final {
        // The name of this landmark.
        std::string name;

        // The position of the landmark in its caller-specified coordinate system.
        Vector3 position;

        friend bool operator==(const NamedLandmark&, const NamedLandmark&) = default;
        friend bool operator==(const Landmark& lhs, const NamedLandmark& rhs)
        {
            return lhs.maybeName == rhs.name && lhs.position == rhs.position;
        }
    };
}
