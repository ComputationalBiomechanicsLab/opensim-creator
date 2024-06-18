#include "MIIDs.h"

using osc::mi::MIIDs;

const MIIDs::AllIDs& osc::mi::MIIDs::GetAllIDs()
{
    static const AllIDs s_AllIDs;
    return s_AllIDs;
}
