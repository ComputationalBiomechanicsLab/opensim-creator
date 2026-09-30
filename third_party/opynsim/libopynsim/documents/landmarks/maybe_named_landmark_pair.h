#pragma once

#include <libopynsim/utilities/landmark_pair_3d.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/c_string_view.h>
#include <SimTKcommon/SmallMatrix.h>

#include <concepts>
#include <optional>
#include <string>
#include <utility>

namespace opyn
{
    // a possibly-not-completely-paired landmark
    class MaybeNamedLandmarkPair final {
    public:
        MaybeNamedLandmarkPair(
            std::string name,
            std::optional<osc::Vector3> maybe_source_position,
            std::optional<osc::Vector3> maybe_destination_position) :

            name_{std::move(name)},
            maybe_source_position_{maybe_source_position},
            maybe_destination_position_{maybe_destination_position}
        {}

        osc::CStringView name() const { return name_; }

        template<std::convertible_to<std::string_view> StringLike>
        void set_name(StringLike&& new_name) { name_ = std::forward<StringLike>(new_name); }

        bool has_source() const { return maybe_source_position_.has_value(); }
        bool has_destination() const { return maybe_destination_position_.has_value(); }
        bool is_fully_paired() const { return has_source() && has_destination(); }
        std::optional<opyn::LandmarkPair3D<float>> try_get_paired_locations() const
        {
            if (maybe_source_position_ && maybe_destination_position_) {
                return opyn::LandmarkPair3D<float>{osc::to<SimTK::fVec3>(*maybe_source_position_), osc::to<SimTK::fVec3>(*maybe_destination_position_)};
            }
            else {
                return std::nullopt;
            }
        }

        void set_destination(std::optional<osc::Vector3> p) { maybe_destination_position_ = p; }
    private:
        std::string name_;
        std::optional<osc::Vector3> maybe_source_position_;
        std::optional<osc::Vector3> maybe_destination_position_;
    };
}
