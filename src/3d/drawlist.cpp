#include "drawlist.hpp"

#include "src/3d/gpu_data_reference.hpp"

#include <algorithm>

static bool optimal_orderering(osmv::Mesh_instance const& m1, osmv::Mesh_instance const& m2) {
    if (m1.rgba.a != m2.rgba.a) {
        // first, sort by opacity descending: opaque elements should be drawn before
        // blended elements
        return m1.rgba.a > m2.rgba.a;
    } else if (m1._meshid != m2._meshid) {
        // second, sort by mesh, because instanced rendering is essentially the
        // process of batching draw calls by mesh
        return m1._meshid < m2._meshid;
    } else if (m1._diffuse_texture != m2._diffuse_texture) {
        // third, sort by texture, because an instanced render call must have
        // one textureset binding for the whole draw call
        return m1._diffuse_texture < m2._diffuse_texture;
    } else {
        // finally, sort by passthrough data
        //
        // logically, this shouldn't matter, but it does because the draw
        // order, when combined with depth testing, might have a downstream
        // effect on selection logic if the passthrough data is being used to
        // select things in screen space (e.g. if two meshes perfectly overlap
        // eachover, we want a consistent approach to selection logic propagation)
        return m1.passthrough_data() < m2.passthrough_data();
    }
}

void osmv::Drawlist::optimize() noexcept {
    std::sort(instances.begin(), instances.end(), optimal_orderering);
}
