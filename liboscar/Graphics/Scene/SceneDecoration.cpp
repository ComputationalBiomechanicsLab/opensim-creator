#include "SceneDecoration.h"

#include <liboscar/Maths/AABBFunctions.h>

using namespace osc;

AABB osc::SceneDecoration::world_space_bounds() const { return transform_aabb(transform, mesh.bounds()); }
