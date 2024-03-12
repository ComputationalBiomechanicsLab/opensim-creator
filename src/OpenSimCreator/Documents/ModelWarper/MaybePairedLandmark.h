#pragma once

#include <OpenSimCreator/Utils/LandmarkPair3D.h>

#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/CStringView.h>

#include <concepts>
#include <optional>
#include <string>
#include <utility>

namespace osc::mow
{
    // a possibly-not-completely-paired landmark
    class MaybePairedLandmark final {
    public:
        MaybePairedLandmark(
            std::string name_,
            std::optional<Vec3> maybeSourcePos_,
            std::optional<Vec3> maybeDestinationPos_) :

            m_Name{std::move(name_)},
            m_MaybeSourcePos{maybeSourcePos_},
            m_MaybeDestinationPos{maybeDestinationPos_}
        {}

        CStringView name() const { return m_Name; }
        CStringView getName() const { return name(); }

        template<std::convertible_to<std::string_view> StringLike>
        void setName(StringLike&& newName) { m_Name = std::forward(newName); }

        bool hasSource() const { return m_MaybeSourcePos.has_value(); }
        bool hasDestination() const { return m_MaybeDestinationPos.has_value(); }
        bool isFullyPaired() const { return hasSource() && hasDestination(); }
        std::optional<LandmarkPair3D> tryGetPairedLocations() const
        {
            if (m_MaybeSourcePos && m_MaybeDestinationPos) {
                return LandmarkPair3D{*m_MaybeSourcePos, *m_MaybeDestinationPos};
            }
            else {
                return std::nullopt;
            }
        }

        void setDestination(std::optional<Vec3> p) { m_MaybeDestinationPos = p; }
    private:
        std::string m_Name;
        std::optional<Vec3> m_MaybeSourcePos;
        std::optional<Vec3> m_MaybeDestinationPos;
    };
}
