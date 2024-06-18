#pragma once

#include <OpenSimCreator/Documents/Landmarks/Landmark.h>

#include <oscar/Maths/Vec3.h>

#include <string>

namespace osc::lm
{
    struct NamedLandmark final {
        std::string name;
        Vec3 position;

        friend bool operator==(const NamedLandmark&, const NamedLandmark&) = default;
        friend bool operator==(const Landmark& lhs, const NamedLandmark& rhs)
        {
            return lhs.maybeName == rhs.name && lhs.position == rhs.position;
        }
    };
}
