#pragma once

#include <libopynsim/documents/landmarks/landmark.h>

#include <liboscar/maths/vector3.h>

#include <string>

namespace opyn
{
    struct NamedLandmark final {
        // The name of this landmark.
        std::string name;

        // The position of the landmark in its caller-specified coordinate system.
        osc::Vector3 position;

        friend bool operator==(const NamedLandmark&, const NamedLandmark&) = default;
        friend bool operator==(const Landmark& lhs, const NamedLandmark& rhs)
        {
            return lhs.maybeName == rhs.name && lhs.position == rhs.position;
        }
    };
}
