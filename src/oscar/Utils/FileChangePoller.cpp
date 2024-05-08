#include "FileChangePoller.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>

namespace
{
    constexpr std::string_view c_model_no_backing_file_sentinel = "Unassigned";

    std::filesystem::file_time_type get_last_modification_time(const std::string& path)
    {
        if (path.empty() or
            path == c_model_no_backing_file_sentinel or
            not std::filesystem::exists(path)) {

            return std::filesystem::file_time_type{};
        }
        else {
            return std::filesystem::last_write_time(path);
        }
    }
}

osc::FileChangePoller::FileChangePoller(
        std::chrono::milliseconds delay_between_checks,
        const std::string& path) :

    delay_between_checks_{delay_between_checks},
    next_polling_time_{std::chrono::system_clock::now() + delay_between_checks},
    file_last_modification_time_{get_last_modification_time(path)},
    enabled_{true}
{}

bool osc::FileChangePoller::change_detected(const std::string& path)
{
    if (not enabled_) {
        // is disabled
        return false;
    }

    if (path.empty() or path == c_model_no_backing_file_sentinel) {
        // has no, or a senteniel, path - do no checks
        return false;
    }

    auto now = std::chrono::system_clock::now();

    if (now < next_polling_time_) {
        // to soon to poll again
        return false;
    }

    if (not std::filesystem::exists(path)) {
        // the file does not exist
        //
        // (e.g. because the user deleted it externally - #495)
        return false;
    }

    auto modification_time = std::filesystem::last_write_time(path);
    next_polling_time_ = now + delay_between_checks_;

    if (modification_time == file_last_modification_time_) {
        return false;
    }

    file_last_modification_time_ = modification_time;

    return true;
}
