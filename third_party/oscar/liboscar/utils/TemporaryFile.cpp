#include "TemporaryFile.h"

#include <liboscar/platform/log.h>
#include <liboscar/platform/os.h>
#include <liboscar/utils/TemporaryFileParameters.h>

#include <filesystem>
#include <fstream>
#include <utility>

using namespace osc;

osc::TemporaryFile::TemporaryFile(const TemporaryFileParameters& params)
{
    auto [stream, path] = mkstemp(params.suffix, params.prefix);
    absolute_path_ = std::move(path);  // NOLINT(cppcoreguidelines-prefer-member-initializer)
    handle_ = std::move(stream);  // NOLINT(cppcoreguidelines-prefer-member-initializer)
}

osc::TemporaryFile::TemporaryFile(TemporaryFile&& tmp) noexcept :
    absolute_path_{std::move(tmp.absolute_path_)},
    handle_{std::move(tmp.handle_)},
    should_delete_{std::exchange(tmp.should_delete_, false)}
{}

TemporaryFile& osc::TemporaryFile::operator=(TemporaryFile&& tmp) noexcept
{
    if (&tmp != this) {
        std::swap(absolute_path_, tmp.absolute_path_);
        std::swap(handle_, tmp.handle_);
        std::swap(should_delete_, tmp.should_delete_);
    }
    return *this;
}

osc::TemporaryFile::~TemporaryFile() noexcept
{
    if (not should_delete_) {
        return;
    }

    if (handle_.is_open()) {
        handle_.close();
    }

    try {
        std::filesystem::remove(absolute_path_);
    }
    catch (const std::filesystem::filesystem_error& ex) {
        log_error("Error closing a temporary file (%s): %s", absolute_path_.string().c_str(), ex.what());
    }
}
