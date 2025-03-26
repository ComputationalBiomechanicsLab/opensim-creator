#pragma once

#include <liboscar/Utils/CStringView.h>

#include <string>
#include <string_view>

namespace osc
{
    // An entry for a list of filters in a file dialog
    class FileDialogFilter final {
    public:
        // Returns a `FileDialogFilter` that allows any file to be selected.
        static FileDialogFilter all_files()
        {
            return FileDialogFilter{"All Files (*.*)", "*"};
        }

        // Constructs an instance of a `FileDialogFilter`:
        //
        // - `name`    a humnan-readable representation of the filter, e.g. "Images"
        // - `pattern` a semicolon-delimited list of file extensions, e.g. "jpg;png;gif"
        explicit FileDialogFilter(std::string_view name, std::string_view pattern) :
            name_{name},
            pattern_{pattern}
        {}

        CStringView name() const { return name_; }
        CStringView pattern() const { return pattern_; }
    private:
        std::string name_;
        std::string pattern_;
    };
}
