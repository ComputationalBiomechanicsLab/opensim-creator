#include "ComponentDecoration.hpp"

#include "src/Graphics/Mesh.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Transform.hpp"

#include <glm/vec4.hpp>

#include <memory>
#include <utility>

osc::ComponentDecoration::ComponentDecoration(std::shared_ptr<Mesh> mesh_,
                                              Transform const& transform_,
                                              glm::vec4 const& color_,
                                              OpenSim::Component const* component_) :
    mesh{std::move(mesh_)},
    transform{transform_},
    color{color_},
    worldspaceAABB{TransformAABB(mesh->getAABB(), transform_)},
    component{std::move(component_)}
{
}
