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
            MeshTopology topology) :

            index_start_{index_start},
            index_count_{index_count},
            topology_{topology}
        {}

        size_t getIndexStart() const { return index_start_; }
        size_t getIndexCount() const { return index_count_; }
        MeshTopology topology() const { return topology_; }

        friend bool operator==(const SubMeshDescriptor&, const SubMeshDescriptor&) = default;
    private:
        size_t index_start_;
        size_t index_count_;
        MeshTopology topology_;
    };
}
