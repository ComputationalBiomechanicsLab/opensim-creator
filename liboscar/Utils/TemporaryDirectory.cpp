#include "TemporaryDirectory.h"

#include <liboscar/Platform/Log.h>
#include <liboscar/Platform/os.h>
#include <liboscar/Utils/TemporaryDirectoryParameters.h>

#include <filesystem>
#include <utility>

using namespace osc;

osc::TemporaryDirectory::TemporaryDirectory(const TemporaryDirectoryParameters& parameters) :
    absolute_path_{mkdtemp(parameters.suffix, parameters.prefix)}
{}
osc::TemporaryDirectory::TemporaryDirectory(TemporaryDirectory&& tmp) noexcept :
    absolute_path_{std::move(tmp.absolute_path_)},
    should_delete_{std::exchange(tmp.should_delete_, false)}
{}
osc::TemporaryDirectory& osc::TemporaryDirectory::operator=(TemporaryDirectory&& tmp) noexcept
{
    if (&tmp != this) {
        std::swap(absolute_path_, tmp.absolute_path_);
        std::swap(should_delete_, tmp.should_delete_);
    }
    return *this;
}
osc::TemporaryDirectory::~TemporaryDirectory() noexcept
{
    if (not should_delete_) {
        return;
    }

    try {
        std::filesystem::remove_all(absolute_path_);
    }
    catch (const std::filesystem::filesystem_error& ex) {
        log_error("Error deleting a temporary directory (%s): %s", absolute_path_.string().c_str(), ex.what());
    }
}
