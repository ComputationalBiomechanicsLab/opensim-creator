#pragma once

#include "3d_common.hpp"
#include "opensim_wrapper.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <filesystem>

namespace osmv {
	// mesh IDs are guaranteed to be globally unique and monotonically increase from 1
	//
	// this guarantee is important because it means that calling code can use direct integer index
	// lookups, rather than having to rely on (e.g.) a runtime hashtable
	using Mesh_id = size_t;

	// one instance of a mesh
	//
	// a model may contain multiple instances of the same mesh
	struct Mesh_instance final {
		glm::mat4 transform;
		glm::mat4 normal_xform;
		glm::vec4 rgba;

		Mesh_id mesh_id;
	};

	// for this API, an OpenSim model's geometry is described as a sequence of rgba-colored mesh
	// instances that are transformed into world coordinates
	struct State_geometry final {
		std::vector<Mesh_instance> mesh_instances;

		void clear() {
			mesh_instances.clear();
		}
	};

	struct Geometry_loader_impl;
	class Geometry_loader final {
		std::unique_ptr<Geometry_loader_impl> impl;

	public:
		Geometry_loader();
		Geometry_loader(Geometry_loader const&) = delete;
		Geometry_loader(Geometry_loader&&);

		Geometry_loader& operator=(Geometry_loader const&) = delete;
		Geometry_loader& operator=(Geometry_loader&&);

		void all_geometry_in(OpenSim::Model& model, SimTK::State& s, State_geometry& out);
		void load_mesh(Mesh_id id, std::vector<Untextured_vert>& out);

		~Geometry_loader() noexcept;
	};
}
