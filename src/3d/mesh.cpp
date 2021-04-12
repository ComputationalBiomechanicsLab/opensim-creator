#include "mesh.hpp"

#include "src/3d/mesh.hpp"
#include "src/3d/untextured_vert.hpp"

#include <glm/ext/matrix_transform.hpp>

#include <cassert>
#include <cmath>
#include <limits>
#include <utility>

using namespace osc;

static bool are_effectively_equal(Untextured_vert const& p1, Untextured_vert const& p2) {
    if (p1.pos == p2.pos) {
        // assumes normals have a magnitude roughly equal to 1
        float cos_angle = glm::dot(p1.normal, p2.normal);
        if (0.99f <= cos_angle && cos_angle <= 1.01f) {
            return true;
        }
    }
    return false;
}

static std::vector<Plain_mesh::element_index_type> create_trivial_indices(size_t n) {
    assert(n < std::numeric_limits<Plain_mesh::element_index_type>::max());

    std::vector<Plain_mesh::element_index_type> rv(n);
    for (size_t i = 0; i < n; ++i) {
        rv[i] = static_cast<Plain_mesh::element_index_type>(i);
    }
    return rv;
}

Plain_mesh Plain_mesh::by_deduping(std::vector<Untextured_vert> points) {
    std::vector<Plain_mesh::element_index_type> indices = create_trivial_indices(points.size());

    for (size_t i = 0; i < points.size(); ++i) {
        Untextured_vert const& p1 = points[i];

        for (size_t j = i + 1; j < points.size(); ++j) {
            Untextured_vert const& p2 = points[j];

            if (!are_effectively_equal(p1, p2)) {
                continue;
            }

            points.erase(points.begin() + j);

            // now update the indices
            for (Plain_mesh::element_index_type& idx : indices) {
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

    return Plain_mesh{std::move(points), std::move(indices)};
}

Plain_mesh Plain_mesh::from_raw_verts(std::vector<Untextured_vert> verts) {
    size_t n = verts.size();
    return Plain_mesh{std::move(verts), create_trivial_indices(n)};
}

Plain_mesh Plain_mesh::from_raw_verts(Untextured_vert const* first, size_t n) {
    return Plain_mesh::from_raw_verts(std::vector<Untextured_vert>(first, first + n));
}

Textured_mesh Textured_mesh::from_raw_verts(std::vector<Textured_vert> verts) {
    size_t n = verts.size();
    return Textured_mesh{std::move(verts), create_trivial_indices(n)};
}

Textured_mesh Textured_mesh::from_raw_verts(Textured_vert const* first, size_t n) {
    return Textured_mesh::from_raw_verts(std::vector<Textured_vert>(first, first + n));
}
