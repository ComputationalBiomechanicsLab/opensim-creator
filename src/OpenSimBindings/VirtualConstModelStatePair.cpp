#include "VirtualConstModelStatePair.hpp"

#include <OpenSim/Common/Component.h>

bool osc::VirtualConstModelStatePair::selectionHasTypeID(std::type_info const& ti) const
{
	OpenSim::Component const* selected = getSelected();
	return selected && typeid(*selected) == ti;
}
