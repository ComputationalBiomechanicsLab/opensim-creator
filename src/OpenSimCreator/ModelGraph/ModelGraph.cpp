#include "ModelGraph.hpp"

#include <OpenSimCreator/ModelGraph/GroundEl.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphIDs.hpp>

#include <oscar/Utils/ClonePtr.hpp>

osc::ModelGraph::ModelGraph() :
    m_Els{{ModelGraphIDs::Ground(), ClonePtr<SceneEl>{GroundEl{}}}}
{
}
