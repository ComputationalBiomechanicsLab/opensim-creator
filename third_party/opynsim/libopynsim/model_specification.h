#pragma once

#include <libopynsim/model.h>

#include <liboscar/utilities/copy_on_upd_ptr.h>

#include <filesystem>
#include <string>

namespace OpenSim { class Model; }

namespace opyn
{
    /// Represents a high-level model specification that can be validated
    /// and compiled into a `Model`.
    ///
    /// Related: https://simtk.org/api_docs/opensim/api_docs32/classOpenSim_1_1Model.html#details
    /// Related: https://opensimconfluence.atlassian.net/wiki/spaces/OpenSim/pages/53089017/SimTK+Simulation+Concepts
    class ModelSpecification {
    public:
        static ModelSpecification from_osim(const std::filesystem::path& source);
        static ModelSpecification example_pendulum();
        static ModelSpecification example_double_pendulum();

        /// Constructs an empty `ModelSpecification`.
        explicit ModelSpecification();

        /// Constructs a `ModelSpecification` from `opensim_model`.
        explicit ModelSpecification(OpenSim::Model&& opensim_model);

        /// Returns a `Model` compiled from this `ModelSpecification`, throws an exception
        /// if there's a compilation error.
        Model compile() const;

        /// Returns the root directory that this `ModelSpecification` uses when
        /// it resolves filesystem resources. That is, complete resource paths
        /// are resolved as `root_directory() / resource_path`.
        std::filesystem::path root_directory() const;

        /// Sets the root directory that this `ModelSpecification` uses
        /// when it resolves filesystem resources.
        void set_root_directory(const std::filesystem::path&);

        /// Writes `*this` to `destination` in an `.osim` format.
        void to_osim(const std::filesystem::path& destination) const;

        /// Returns `*this` written to a `std::string` in an `.osim` format.
        std::string to_osim() const;

        /// Converts any `StationDefinedFrame`s in `*this` into `PhysicalOffsetFrame`s.
        ///
        /// Can be useful for compatibility with OpenSim <4.6, which doesn't
        /// support `StationDefinedFrame`s.
        void bake_station_defined_frames();

        /// Flushes all in-memory resources in `*this` to `directory` and updates
        /// the associated components to point to the flushed resources.
        void flush_in_memory_resources_to(const std::filesystem::path& directory);
    private:
        class Impl;
        explicit ModelSpecification(osc::CopyOnUpdPtr<Impl>);

        osc::CopyOnUpdPtr<Impl> impl_;
    };
}
