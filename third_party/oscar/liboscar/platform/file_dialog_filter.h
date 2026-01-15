#pragma once

#include <liboscar/utils/CStringView.h>

#include <concepts>
#include <string>
#include <utility>

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
        template<typename StringLike1, typename StringLike2>
        requires
            std::constructible_from<std::string, StringLike1&&> and
            std::constructible_from<std::string, StringLike2&&>
        explicit FileDialogFilter(StringLike1&& name, StringLike2&& pattern) :
            name_{std::forward<StringLike1>(name)},
            pattern_{std::forward<StringLike2>(pattern)}
        {}

        CStringView name() const { return name_; }
        CStringView pattern() const { return pattern_; }
    private:
        std::string name_;
        std::string pattern_;
    };
}
