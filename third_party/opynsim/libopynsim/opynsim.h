#pragma once

#include <libopynsim/model_specification.h>

#include <liboscar/platform/log_level.h>

#include <filesystem>

namespace opyn
{
    /// Set the logging level of the default global log sink that's used by all subsystems
    ///
    /// This may be called before `init`.
    void set_log_level(osc::LogLevel);

    /// Globally initializes the opynsim (oscar + OpenSim + Simbody + extensions) API.
    ///
    /// This should be called by the application before using any `opyn::`, `SimTK::`, or
    /// `OpenSim::`-prefixed API. A process may call it multiple times, but only the first
    /// call will actually do anything.
    bool init();

    /// Globally adds `directory` to the list of geometry directories that OpenSim
    /// will use as a fallback when trying to find mesh files referenced by model files
    /// (e.g. pelvis.vtp).
    void add_opensim_geometry_directory(const std::filesystem::path& directory);

    /// Returns a `ModelSpecification` imported from `osim_file_path`, throws if there's an
    /// import error.
    ModelSpecification import_osim_file(const std::filesystem::path& osim_file_path);

    /// Returns a `Model` compiled from the given `ModelSpecification`, throws an exception
    /// if there's a compilation error.
    Model compile_specification(const ModelSpecification&);
}
