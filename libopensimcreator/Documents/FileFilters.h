#pragma once

#include <liboscar/platform/FileDialogFilter.h>

#include <span>

namespace osc
{
    std::span<const FileDialogFilter> GetOpenSimXMLFileFilters();
    std::span<const FileDialogFilter> GetModelFileFilters();
    std::span<const FileDialogFilter> GetMotionFileFilters();
    std::span<const FileDialogFilter> GetMotionFileFiltersIncludingTRC();
}
