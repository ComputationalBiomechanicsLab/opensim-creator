#include "ModelGraphIDs.hpp"

osc::ModelGraphIDs::AllIDs const& osc::ModelGraphIDs::GetAllIDs()
{
    static AllIDs const s_AllIDs;
    return s_AllIDs;
}
