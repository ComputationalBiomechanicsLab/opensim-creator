#include "FileFilters.h"

#include <liboscar/Platform/FileDialogFilter.h>

#include <array>
#include <span>

using namespace osc;

std::span<const FileDialogFilter> osc::GetOpenSimXMLFileFilters()
{
    static const auto s_Filters = std::to_array<FileDialogFilter>({
        FileDialogFilter::all_files(),
        FileDialogFilter{"Extensible Markup Language (*.xml)", "xml"},
    });
    return s_Filters;
}

std::span<const FileDialogFilter> osc::GetModelFileFilters()
{
    static const auto s_Filters = std::to_array<FileDialogFilter>({
        FileDialogFilter::all_files(),
        FileDialogFilter{"OpenSim Model File (*.osim)", "osim"},
    });
    return s_Filters;
}

std::span<const FileDialogFilter> osc::GetMotionFileFilters()
{
    static const auto s_Filters = std::to_array<FileDialogFilter>({
        FileDialogFilter::all_files(),
        FileDialogFilter{"Motion Data (*.sto, *.mot)", "sto;mot"},
        FileDialogFilter{"OpenSim Storage File (*.sto)", "sto"},
        FileDialogFilter{"OpenSim/SIMM Motion File (*.mot)", "mot"},
    });
    return s_Filters;
}

std::span<const FileDialogFilter> osc::GetMotionFileFiltersIncludingTRC()
{
    static const auto s_Filters = std::to_array<FileDialogFilter>({
        FileDialogFilter::all_files(),
        FileDialogFilter{"Motion Data (*.sto, *.mot)", "sto;mot"},
        FileDialogFilter{"OpenSim Storage File (*.sto)", "sto"},
        FileDialogFilter{"OpenSim/SIMM Motion File (*.mot)", "mot"},
        FileDialogFilter{"Track Row Column File (*.trc)", "trc"},
    });
    return s_Filters;
}
