#include "raw_drawlist.hpp"

#include <algorithm>

static bool is_opaque(osmv::Raw_mesh_instance const& m) {
    return m.rgba.a >= 1.0f;
}

static bool has_lower_meshid(osmv::Raw_mesh_instance const& m1, osmv::Raw_mesh_instance const& m2) {
    return m1._meshid < m2._meshid;
}

void osmv::Raw_drawlist::optimize() noexcept {
    auto blended_start = std::partition(instances.begin(), instances.end(), is_opaque);
    std::sort(instances.begin(), blended_start, has_lower_meshid);
    std::sort(blended_start, instances.end(), has_lower_meshid);
}
