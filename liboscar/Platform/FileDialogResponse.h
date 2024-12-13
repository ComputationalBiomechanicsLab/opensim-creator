#pragma once

#include <liboscar/Utils/CStringView.h>

#include <cstddef>
#include <filesystem>
#include <string_view>
#include <vector>

namespace osc
{
    class FileDialogResponse final {
    public:
        explicit FileDialogResponse(std::string_view error) :
            error_{error}
        {}

        explicit FileDialogResponse(std::vector<std::filesystem::path> filelist) :
            filelist_{std::move(filelist)}
        {}

        bool has_error() const { return not error_.empty(); }
        CStringView error() const { return error_; }
        size_t size() const { return filelist_.size(); }
        auto begin() const { return filelist_.begin(); }
        auto end() const { return filelist_.end(); }
    private:
        std::string error_;
        std::vector<std::filesystem::path> filelist_;
    };
}
