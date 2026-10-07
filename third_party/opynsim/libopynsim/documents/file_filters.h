#pragma once

#include <liboscar/platform/file_dialog_filter.h>

#include <span>

namespace opyn
{
    std::span<const osc::FileDialogFilter> get_open_sim_xml_file_filters();
    std::span<const osc::FileDialogFilter> get_model_file_filters();
    std::span<const osc::FileDialogFilter> get_motion_file_filters();
    std::span<const osc::FileDialogFilter> get_motion_file_filters_including_trc();
}
