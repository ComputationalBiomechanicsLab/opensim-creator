#include "raw_drawlist.hpp"

#include <algorithm>

void osmv::Raw_drawlist::optimize() noexcept {
    std::sort(
        instances.begin(), instances.end(), [](osmv::Raw_mesh_instance const& a, osmv::Raw_mesh_instance const& b) {
            return a.rgba.a != b.rgba.a ? a.rgba.a > a.rgba.b : a._meshid < b._meshid;
        });
}
