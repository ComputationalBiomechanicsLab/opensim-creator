#include "TemporaryFile.h"

#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/Utils/TemporaryFileParameters.h>

#include <filesystem>
#include <fstream>
#include <utility>

using namespace osc;

osc::TemporaryFile::TemporaryFile(const TemporaryFileParameters& params)
{
    auto [stream, path] = mkstemp(params.suffix, params.prefix);
    absolute_path_ = std::move(path);
    handle_ = std::move(stream);
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
        catch (const std::filesystem::filesystem_error& ex) {
            log_error("error closing a temporary file, this could be a sign of operating system issues: %s", ex.what());
        }
    }
}
