#pragma once

#include <libopynsim/model_specification.h>

#include <liboscar/platform/log_level.h>

#include <filesystem>
#include <vector>

namespace opyn
{
    /// Set the logging level of the default global log sink that's used by all subsystems
    ///
    /// This is the only `opyn` function that may be called before `init`.
    void set_log_level(osc::LogLevel);

    /// Globally initializes the opynsim API.
    ///
    /// This should be called by the application before using any `opyn::`, `SimTK::`, or
    /// `OpenSim::`-prefixed API. A process may call it multiple times, but only the first
    /// call will actually do anything.
    bool init();

    /// Returns the global search paths that OPynSim uses to resolve relative
    /// resource paths (e.g. mesh files) in high-to-low priority order.
    ///
    /// When resolving `path`, each attempt is resolved as `base_directory / search_path / path`,
    /// where `base_directory` is context-dependent (e.g. loading a resource referenced in a
    /// file would use the file's directory as a base). Absolute search paths overrule
    /// the base directory (same as `std::filesystem::path::append`).
    std::vector<std::filesystem::path> get_search_paths();

    /// Prepends `search_path` to the start (highest-priority) of the global
    /// search path sequence (see `get_search_paths`).
    ///
    /// - If `search_path` is already exists in the search path sequence, it is moved
    ///   to the start of the sequence.
    /// - `search_path` does not need to exist on the filesystem.
    void prepend_search_path(std::filesystem::path search_path);

    /// Appends `search_path` to the end (lowest-priority) of the global
    /// search path sequence (see `get_search_paths`).
    ///
    /// - If `search_path` is already exists in the search path sequence, it is moved
    ///   to the end of the sequence.
    /// - `search_path` does not need to exist on the filesystem.
    void append_search_path(std::filesystem::path search_path);

    /// Returns `true` if `search_path` was found and removed from the global
    /// search path sequence.
    bool remove_search_path(const std::filesystem::path& search_path);

    /// Returns a `ModelSpecification` imported from `osim_file_path`, throws if there's an
    /// import error.
    ModelSpecification import_osim_file(const std::filesystem::path& osim_file_path);

    /// Returns a `ModelSpecification` for a simple pendulum.
    ModelSpecification example_specification_pendulum();

    /// Returns a `ModelSpecification` for an example double pendulum.
    ModelSpecification example_specification_double_pendulum();

    /// Returns `model_specification.compile()`.
    Model compile_specification(const ModelSpecification& model_specification);
}
