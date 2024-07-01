#include "TemporaryFile.h"

#include <oscar/Platform/os.h>
#include <oscar/Utils/TemporaryFileParameters.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>

using namespace osc;

osc::TemporaryFile::TemporaryFile(const TemporaryFileParameters& params)
{
    auto p = mkstemp(params.suffix, params.prefix);
    absolute_path_ = std::move(p.second);
    handle_ = std::move(p.first);
}

osc::TemporaryFile::TemporaryFile(TemporaryFile&& tmp) noexcept :
    absolute_path_{std::move(tmp.absolute_path_)},
    handle_{std::move(tmp.handle_)},
    should_delete_{std::exchange(tmp.should_delete_, false)}
{}
TemporaryFile& osc::TemporaryFile::operator=(TemporaryFile&& tmp) noexcept
{
    using std::swap;

    if (&tmp == this) {
        return *this;
    }

    swap(absolute_path_, tmp.absolute_path_);
    swap(handle_, tmp.handle_);
    swap(should_delete_, tmp.should_delete_);
    return *this;
}
osc::TemporaryFile::~TemporaryFile() noexcept
{
    if (should_delete_) {
        if (handle_.is_open()) {
            handle_.close();
        }
        try {
            std::filesystem::remove(absolute_path_);
        }
        catch (const std::filesystem::filesystem_error&) {
            // swallow it: we're in a destructor, so the best we could hope for
            //             is to report it to the log or similar
        }
    }
}
