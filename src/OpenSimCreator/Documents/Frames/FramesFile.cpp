#include "FramesFile.hpp"

#include <algorithm>

using osc::frames::FrameDefinition;

FrameDefinition const* osc::frames::FramesFile::findFrameDefinitionByName(std::string_view name) const
{
    auto const it = std::find_if(m_FrameDefs.begin(), m_FrameDefs.end(), [name](auto const& fd)
    {
        return fd.getName() == name;
    });

    if (it != m_FrameDefs.end())
    {
        return &(*it);
    }
    else
    {
        return nullptr;
    }
}
