#pragma once

#include <filesystem>

// os: where all the icky os-specific stuff is hidden
//
// - e.g. resource paths, configuration loading, current exe location
namespace osmv {
	std::filesystem::path resource_path(std::filesystem::path const&);

}