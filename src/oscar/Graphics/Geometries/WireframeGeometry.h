#pragma once

#include <oscar/Graphics/Mesh.h>

namespace osc
{
	class WireframeGeometry final {
	public:
		static Mesh generate_mesh(Mesh const&);
	};
}
