#include "SceneMesh.hpp"

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/BVH.hpp>

#include <memory>

using osc::BVH;
using osc::Mesh;
using osc::MeshTopology;

namespace
{
    BVH CreateTriangleBVH(Mesh const& mesh)
    {
        BVH rv;

        auto const indices = mesh.getIndices();

        if (indices.empty())
        {
            return rv;
        }
        else if (mesh.getTopology() != MeshTopology::Triangles)
        {
            return rv;
        }
        else if (indices.isU32())
        {
            rv.buildFromIndexedTriangles(mesh.getVerts() , indices.toU32Span());
        }
        else
        {
            rv.buildFromIndexedTriangles(mesh.getVerts(), indices.toU16Span());
        }

        return rv;
    }
}

osc::SceneMesh::SceneMesh()
{
    static std::shared_ptr<Data const> const s_DedupedDefaultData = std::make_shared<Data>();
    m_Data = s_DedupedDefaultData;
}

osc::SceneMesh::SceneMesh(Mesh const& mesh_) :
    m_Data{std::make_shared<Data>(mesh_, CreateTriangleBVH(mesh_))}
{
}
