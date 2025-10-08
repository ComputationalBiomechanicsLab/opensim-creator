#pragma once

#include <libopensimcreator/Utils/LandmarkPair3D.h>

#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/CStringView.h>

#include <concepts>
#include <optional>
#include <string>
#include <utility>

namespace osc
{
    // a possibly-not-completely-paired landmark
    class MaybeNamedLandmarkPair final {
    public:
        MaybeNamedLandmarkPair(
            std::string name_,
            std::optional<Vector3> maybeSourcePosition,
            std::optional<Vector3> maybeDestinationPosition) :

            m_Name{std::move(name_)},
            m_MaybeSourcePosition{maybeSourcePosition},
            m_MaybeDestinationPosition{maybeDestinationPosition}
        {}

        CStringView name() const { return m_Name; }
        CStringView getName() const { return name(); }

        template<std::convertible_to<std::string_view> StringLike>
        void setName(StringLike&& newName) { m_Name = std::forward<StringLike>(newName); }

        bool hasSource() const { return m_MaybeSourcePosition.has_value(); }
        bool hasDestination() const { return m_MaybeDestinationPosition.has_value(); }
        bool isFullyPaired() const { return hasSource() && hasDestination(); }
        std::optional<LandmarkPair3D<float>> tryGetPairedLocations() const
        {
            if (m_MaybeSourcePosition && m_MaybeDestinationPosition) {
                return LandmarkPair3D<float>{*m_MaybeSourcePosition, *m_MaybeDestinationPosition};
            }
            else {
                return std::nullopt;
            }
        }

        void setDestination(std::optional<Vector3> p) { m_MaybeDestinationPosition = p; }
    private:
        std::string m_Name;
        std::optional<Vector3> m_MaybeSourcePosition;
        std::optional<Vector3> m_MaybeDestinationPosition;
    };
}
