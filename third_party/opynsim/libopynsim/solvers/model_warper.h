#pragma once

#include <liboscar/utilities/copy_on_upd_shared_value.h>

#include <cstddef>
#include <filesystem>

namespace opyn { class ModelSpecification; }

namespace opyn
{
    /// A solver that can warp `ModelSpecification`s by applying a sequence of
    /// `ScalingStep`s to an input `ModelSpecification`.
    class ModelWarper final {
    public:
        /// Constructs a blank `ModelWarper` with no `ScalingStep`s or scaling
        /// parameters.
        explicit ModelWarper();

        /// Constructs a `ModelWarper` by loading its data from `source`, which
        /// is typically an XML document that deserializes into a `ModelWarperV3Document`
        /// (legacy, from OpenSim Creator). Throws  an exception if there is an
        /// IO/parsing/validation error.
        explicit ModelWarper(const std::filesystem::path& source);

        /// Returns the number of `ScalingStep`s performed by this `ModelWarper` when it
        /// `warp`s a `ModelSpecification`.
        size_t num_scaling_steps() const;

        /// Returns the number of scaling parameters that this `ModelWarper` is parameterized by.
        size_t num_scaling_parameters() const;

        /// Warps `model_specification` and writes the warped version to `osim_output_path`,
        /// ensuring warped geometries are written to `warped_geometry_directory` and are
        /// correctly referenced (relative/absolute paths) in the output osim.
        ///
        /// @param model_specification The source `ModelSpecification` that should
        /// be copied and warped by this `ModelWarper`.
        /// @param osim_output_path A filesystem path where the warped `ModelSpecification`
        /// should be written as an osim.
        /// @param warped_geometry_directory A path, relative to `osim_output_path`, where
        /// warped geometry should be written. The directory will be created if it does
        /// not exist.
        /// @param bake_station_defined_frames if `true`, the implementation will "bake"
        /// any `StationDefinedFrame`s in `model_specification` into `PhysicalOffsetFrame`s.
        /// This can be necessary when the model is to be used with OpenSim < v4.6.
        void warp_to_osim_file(
            const ModelSpecification& model_specification,
            const std::filesystem::path& osim_output_path,
            const std::filesystem::path& warped_geometry_directory = "WarpedGeometry",
            bool bake_station_defined_frames = false
        ) const;
    private:
        class Impl;
        osc::CopyOnUpdSharedValue<Impl> impl_;
    };
}
