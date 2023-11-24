#include "ModelGraph.hpp"

#include <OpenSimCreator/Documents/ModelGraph/GroundEl.hpp>
#include <OpenSimCreator/Documents/ModelGraph/ModelGraphIDs.hpp>

#include <oscar/Utils/ClonePtr.hpp>

osc::ModelGraph::ModelGraph() :
    m_Els{{ModelGraphIDs::Ground(), ClonePtr<SceneEl>{GroundEl{}}}}
{
}
