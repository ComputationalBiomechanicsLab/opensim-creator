#include "mesh.hpp"

#include "src/3d/mesh.hpp"
#include "src/3d/untextured_vert.hpp"

#include <glm/ext/matrix_transform.hpp>

#include <cmath>
#include <utility>

using namespace osmv;

static bool are_effectively_equal(Untextured_vert const& p1, Untextured_vert const& p2) {
    if (p1.pos == p2.pos) {
        // assumes normals have a magnitude roughly equal to 1
        float cos_angle = glm::dot(p1.normal, p2.normal);
        if (0.99f <= cos_angle and cos_angle <= 1.01f) {
            return true;
        }
    }
    return false;
}

Plain_mesh Plain_mesh::by_deduping(std::vector<Untextured_vert> points) {
    Plain_mesh pm;
    pm.vert_data = std::move(points);
    pm.generate_trivial_indices();

    for (size_t i = 0; i < pm.vert_data.size(); ++i) {
        Untextured_vert const& p1 = pm.vert_data[i];

        for (size_t j = i + 1; j < pm.vert_data.size(); ++j) {
            Untextured_vert const& p2 = pm.vert_data[j];

            if (not are_effectively_equal(p1, p2)) {
                continue;
            }

            pm.vert_data.erase(pm.vert_data.begin() + j);

            // now update the indices
            for (Plain_mesh::element_index_type& idx : pm.indices) {
                if (idx == j) {
                    idx = static_cast<Plain_mesh::element_index_type>(i);
                } else if (idx > j) {
                    // erasing has slid everything down one place, so adjust
                    // those indices accordingly
                    idx--;
                }
            }
        }
    }

    return pm;
}
