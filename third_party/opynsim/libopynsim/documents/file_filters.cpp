#include "file_filters.h"

#include <liboscar/platform/file_dialog_filter.h>

#include <array>
#include <span>

using namespace osc;

std::span<const FileDialogFilter> opyn::get_open_sim_xml_file_filters()
{
    static const auto s_Filters = std::to_array<FileDialogFilter>({
        FileDialogFilter{"Extensible Markup Language (*.xml)", "xml"},
        FileDialogFilter::all_files(),
    });
    return s_Filters;
}

std::span<const FileDialogFilter> opyn::get_model_file_filters()
{
    static const auto s_Filters = std::to_array<FileDialogFilter>({
        FileDialogFilter{"OpenSim Model File (*.osim)", "osim"},
        FileDialogFilter::all_files(),
    });
    return s_Filters;
}

std::span<const FileDialogFilter> opyn::get_motion_file_filters()
{
    static const auto s_Filters = std::to_array<FileDialogFilter>({
        FileDialogFilter{"Motion Data (*.sto, *.mot)", "sto;mot"},
        FileDialogFilter{"OpenSim Storage File (*.sto)", "sto"},
        FileDialogFilter{"OpenSim/SIMM Motion File (*.mot)", "mot"},
        FileDialogFilter::all_files(),
    });
    return s_Filters;
}

std::span<const FileDialogFilter> opyn::get_motion_file_filters_including_trc()
{
    static const auto s_Filters = std::to_array<FileDialogFilter>({
        FileDialogFilter{"Motion Data (*.sto, *.mot, *.trc)", "sto;mot;trc"},
        FileDialogFilter{"OpenSim Storage File (*.sto)", "sto"},
        FileDialogFilter{"OpenSim/SIMM Motion File (*.mot)", "mot"},
        FileDialogFilter{"Track Row Column File (*.trc)", "trc"},
        FileDialogFilter::all_files(),
    });
    return s_Filters;
}
