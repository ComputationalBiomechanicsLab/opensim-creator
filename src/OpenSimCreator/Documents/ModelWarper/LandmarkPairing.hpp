#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <optional>
#include <string>
#include <utility>

namespace osc::mow
{
    class LandmarkPairing final {
    public:
        LandmarkPairing(
            std::string name_,
            std::optional<Vec3> maybeSourcePos_,
            std::optional<Vec3> maybeDestinationPos_) :

            m_Name{std::move(name_)},
            m_MaybeSourcePos{maybeSourcePos_},
            m_MaybeDestinationPos{maybeDestinationPos_}
        {
        }

    private:
        std::string m_Name;
        std::optional<Vec3> m_MaybeSourcePos;
        std::optional<Vec3> m_MaybeDestinationPos;
    };
}
