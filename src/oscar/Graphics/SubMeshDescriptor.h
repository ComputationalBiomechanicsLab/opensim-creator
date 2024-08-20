#pragma once

#include <oscar/Graphics/MeshTopology.h>

#include <cstddef>

namespace osc
{
    class SubMeshDescriptor final {
    public:
        SubMeshDescriptor(
            size_t index_start,
            size_t index_count,
            MeshTopology topology,
            size_t base_vertex = 0) :

            index_start_{index_start},
            index_count_{index_count},
            topology_{topology},
            base_vertex_{base_vertex}
        {}

        friend bool operator==(const SubMeshDescriptor&, const SubMeshDescriptor&) = default;

        size_t index_start() const { return index_start_; }
        size_t index_count() const { return index_count_; }
        MeshTopology topology() const { return topology_; }
        size_t base_vertex() const { return base_vertex_; }
    private:
        size_t index_start_;
        size_t index_count_;
        MeshTopology topology_;
        size_t base_vertex_;
    };
}
