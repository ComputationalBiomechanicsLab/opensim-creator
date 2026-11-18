#include "mi_ids.h"

using osc::MiIDs;

const MiIDs::AllIDs& osc::MiIDs::get_all_ids()
{
    static const AllIDs s_all_ids;
    return s_all_ids;
}
