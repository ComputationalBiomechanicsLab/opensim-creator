#include "MIIDs.h"

using osc::mi::MIIDs;

MIIDs::AllIDs const& osc::mi::MIIDs::GetAllIDs()
{
    static AllIDs const s_AllIDs;
    return s_AllIDs;
}
