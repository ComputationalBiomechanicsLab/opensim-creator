#pragma once

#include <liboscar/platform/file_dialog_filter.h>

#include <span>

namespace opyn
{
    std::span<const osc::FileDialogFilter> GetOpenSimXMLFileFilters();
    std::span<const osc::FileDialogFilter> GetModelFileFilters();
    std::span<const osc::FileDialogFilter> GetMotionFileFilters();
    std::span<const osc::FileDialogFilter> GetMotionFileFiltersIncludingTRC();
}
