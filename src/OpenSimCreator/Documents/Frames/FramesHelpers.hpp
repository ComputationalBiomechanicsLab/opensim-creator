#pragma once

#include <OpenSimCreator/Documents/Frames/FramesFile.hpp>

#include <iosfwd>

namespace osc::frames
{
    FramesFile ReadFramesFromTOML(std::istream&);
}
