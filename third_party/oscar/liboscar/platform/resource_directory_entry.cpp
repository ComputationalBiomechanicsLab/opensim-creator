#include "resource_directory_entry.h"

#include <liboscar/utils/HashHelpers.h>

#include <ostream>

std::ostream& osc::operator<<(std::ostream& lhs, const ResourceDirectoryEntry& rhs)
{
    lhs << "ResourceDirectoryEntry{path = " << rhs.path() << ", is_directory = " << rhs.is_directory() << '}';
    return lhs;
}

size_t std::hash<osc::ResourceDirectoryEntry>::operator()(const osc::ResourceDirectoryEntry& e) const
{
    return osc::hash_of(e.path(), e.is_directory());
}
