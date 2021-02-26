#include "drawlist.hpp"

#include "src/3d/gpu_data_reference.hpp"

#include <algorithm>

static bool is_opaque(osmv::Mesh_instance const& m) {
    return m.rgba.a >= 1.0f;
}

static bool optimal_orderering(osmv::Mesh_instance const& m1, osmv::Mesh_instance const& m2) {
    return m1._diffuse_texture != m2._diffuse_texture ? m1._diffuse_texture < m2._diffuse_texture
                                                      : m1._meshid < m2._meshid;
}

void osmv::Drawlist::optimize() noexcept {
    auto blended_start = std::partition(instances.begin(), instances.end(), is_opaque);
    std::sort(instances.begin(), blended_start, optimal_orderering);
    std::sort(blended_start, instances.end(), optimal_orderering);
}
