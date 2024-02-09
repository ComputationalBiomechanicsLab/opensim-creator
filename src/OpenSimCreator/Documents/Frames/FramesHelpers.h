#pragma once

#include <OpenSimCreator/Documents/Frames/FramesFile.h>

#include <iosfwd>

namespace osc::frames
{
    FramesFile ReadFramesFromTOML(std::istream&);
}
