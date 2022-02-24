#include "ComponentDecoration.hpp"

#include "src/3D/Mesh.hpp"
#include "src/3D/Model.hpp"

#include <glm/vec4.hpp>

#include <utility>

osc::ComponentDecoration::ComponentDecoration(std::shared_ptr<Mesh> mesh_,
                                              Transform const& transform_,
                                              glm::vec4 const& color_,
                                              OpenSim::Component const* component_) :
    mesh{std::move(mesh_)},
    modelMtx{toMat4(transform_)},
    normalMtx{toNormalMatrix(transform_)},
    color{color_},
    worldspaceAABB{AABBApplyXform(mesh->getAABB(), transform_)},
    component{std::move(component_)}
{
}
