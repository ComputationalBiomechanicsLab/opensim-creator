#pragma once

#include <memory>

namespace osc { struct AABB; }
namespace osc { class BVH; }
namespace osc { class Mesh; }

namespace osc
{
	class SceneMesh final {
	public:
		explicit SceneMesh(Mesh const&);

		operator Mesh const& () const
		{
			return getMesh();
		}
		Mesh const& getMesh() const;
		AABB const& getBounds() const;
		BVH const& getBVH() const;
	private:
		class Impl;
		std::shared_ptr<Impl const> m_Impl;
	};
}
