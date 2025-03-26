#pragma once

#include <liboscar/Utils/CStringView.h>

#include <cstddef>
#include <filesystem>
#include <string_view>
#include <vector>

namespace osc
{
    // Represents a response from a file dialog (Open, Save, Save As, etc.), where
    // the response can be either:
    //
    // - An error (e.g. an OS error that prevents the dialog from working).
    // - A list of paths that the user selected, or no paths if the user cancelled out
    //   of the dialog.
    class FileDialogResponse final {
    public:
        // Constructs a `FileDialogResponse` that represents an error.
        explicit FileDialogResponse(std::string_view error) :
            error_{error}
        {}

        // Constructs a `FileDialogResponse` that represents a user selection of `filelist`.
        explicit FileDialogResponse(std::vector<std::filesystem::path> filelist) :
            filelist_{std::move(filelist)}
        {}

        bool has_error() const { return not error_.empty(); }
        CStringView error() const { return error_; }
        size_t size() const { return filelist_.size(); }
        auto begin() const { return filelist_.begin(); }
        auto end() const { return filelist_.end(); }
        const std::filesystem::path& front() const { return *begin(); }
    private:
        std::string error_;
        std::vector<std::filesystem::path> filelist_;
    };
}
