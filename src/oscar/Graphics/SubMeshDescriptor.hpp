#pragma once

#include <oscar/Graphics/MeshTopology.hpp>

#include <cstddef>

namespace osc
{
    class SubMeshDescriptor final {
    public:
        SubMeshDescriptor(
            size_t indexStart_,
            size_t indexCount_,
            MeshTopology topology_) :

            m_IndexStart{indexStart_},
            m_IndexCount{indexCount_},
            m_Topology{topology_}
        {
        }

        size_t getIndexStart() const
        {
            return m_IndexStart;
        }

        size_t getIndexCount() const
        {
            return m_IndexCount;
        }

        MeshTopology getTopology() const
        {
            return m_Topology;
        }

        friend bool operator==(SubMeshDescriptor const&, SubMeshDescriptor const&) = default;
    private:
        size_t m_IndexStart;
        size_t m_IndexCount;
        MeshTopology m_Topology;
    };
}
