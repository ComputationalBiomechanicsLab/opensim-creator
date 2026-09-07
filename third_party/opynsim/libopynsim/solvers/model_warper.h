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
        /// Returns a `ModelWarper` read from `source`, which should be a path
        /// to an XML document that contains a `ModelWarperV3Document` (a legacy
        /// data structure, from OpenSim Creator's model warper). Throws an exception
        /// if there is an IO/parsing/validation error.
        static ModelWarper from_xml(const std::filesystem::path& source);

        /// Constructs a blank `ModelWarper` with no `ScalingStep`s or scaling
        /// parameters.
        explicit ModelWarper();

        /// Returns the number of `ScalingStep`s performed by this `ModelWarper` when it
        /// `warp`s a `ModelSpecification`.
        size_t num_scaling_steps() const;

        /// Returns the number of scaling parameters that this `ModelWarper` is parameterized by.
        size_t num_scaling_parameters() const;

        /// Returns a warped copy of `model_specification`.
        ///
        /// @param model_specification The source `ModelSpecification` that should
        /// be copied and warped by this `ModelWarper`.
        ModelSpecification warp(const ModelSpecification& model_specification) const;
    private:
        class Impl;
        explicit ModelWarper(osc::CopyOnUpdSharedValue<Impl> impl);

        osc::CopyOnUpdSharedValue<Impl> impl_;
    };
}
