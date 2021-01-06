#include "os.hpp"

#include <fstream>
#include <toml.hpp>
#include <Windows.h>
#include <libloaderapi.h>

namespace fs = std::filesystem;

static std::filesystem::path current_exe_path() {
	char buf[2048];
	if (not GetModuleFileNameA(nullptr, buf, sizeof(buf))) {
		throw std::runtime_error{ "could not get a path to the current executable - the path may be too long? (max: 2048 chars)" };
	}
	return std::filesystem::path{ buf };
}

struct App_config final {
	std::filesystem::path resource_dir;
};

static App_config load_application_config() {
	// the "system-wide" application config is searched recursively by stepping
	// up the directory tree, searching for `osmv.toml`. 
	//
	// If it is found, then the values in that file are used. If not, then use
	// reasonable defaults.
	//
	// note: for development, a config file is generated which hard-codes the
	//       absolute path to the developer's resource dir into the config file
	//       so that devs don't have to copy things around while developing

	fs::path p = current_exe_path().parent_path();
	bool exists = false;
	while (p.has_filename()) {
		fs::path maybe_config = p / "osmv.toml";
		if (fs::exists(maybe_config)) {
			p = maybe_config;
			exists = true;
			break;
		}
		p = p.parent_path();
	}

	// no config: return an in-memory config that has reasonable defaults
	if (not exists) {
		fs::path default_resource_dir = fs::path{ ".." } / "resources";
		return App_config{ default_resource_dir };
	}

	// warning: can throw
	auto config = toml::parse_file(p.c_str());	
	std::string_view resource_dir = config["resources"].value_or("../resources");

	// configuration resource_dir is relative *to the configuration file*
	fs::path config_file_dir = p.parent_path();
	fs::path resource_dir_path = config_file_dir / resource_dir;

	return App_config{ resource_dir_path };
}

struct App_config config() {
	static const App_config config = load_application_config();
	return config;
}

std::filesystem::path osmv::resource_path(std::filesystem::path const& subpath) {
	return  config().resource_dir / subpath;
}
