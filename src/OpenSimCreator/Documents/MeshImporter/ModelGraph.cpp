#include "ModelGraph.hpp"

#include <OpenSimCreator/Documents/MeshImporter/GroundEl.hpp>
#include <OpenSimCreator/Documents/MeshImporter/ModelGraphIDs.hpp>

#include <oscar/Utils/ClonePtr.hpp>

osc::ModelGraph::ModelGraph() :
    m_Els{{ModelGraphIDs::Ground(), ClonePtr<SceneEl>{GroundEl{}}}}
{
}
