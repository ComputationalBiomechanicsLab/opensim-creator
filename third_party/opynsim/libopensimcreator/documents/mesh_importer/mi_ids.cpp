#include "mi_ids.h"

using osc::MiIDs;

const MiIDs::AllIDs& osc::MiIDs::GetAllIDs()
{
    static const AllIDs s_AllIDs;
    return s_AllIDs;
}
