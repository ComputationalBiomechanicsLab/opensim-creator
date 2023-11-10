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

class osc::SceneMesh::Impl final {
public:
	explicit Impl(Mesh const& mesh_) :
		m_Mesh{mesh_},
		m_BVH{CreateTriangleBVH(mesh_)}
	{
	}

	Mesh const& getMesh() const
	{
		return m_Mesh;
	}

	AABB const& getBounds() const
	{
		return m_Mesh.getBounds();
	}

	BVH const& getBVH() const
	{
		return m_BVH;
	}
private:
	Mesh m_Mesh;
	BVH m_BVH;
};

osc::SceneMesh::SceneMesh(Mesh const& mesh_) :
	m_Impl{std::make_shared<Impl>(mesh_)}
{
}

osc::Mesh const& osc::SceneMesh::getMesh() const
{
	return m_Impl->getMesh();
}

osc::AABB const& osc::SceneMesh::getBounds() const
{
	return m_Impl->getBounds();
}

osc::BVH const& osc::SceneMesh::getBVH() const
{
	return m_Impl->getBVH();
}
