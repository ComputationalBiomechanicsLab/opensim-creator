#include "file_backed_storage.h"

#include <libopynsim/utilities/open_sim_helpers.h>
#include <OpenSim/Common/Storage.h>

#include <filesystem>
#include <memory>
#include <utility>

using namespace opyn;

opyn::FileBackedStorage::FileBackedStorage(const OpenSim::Model& model, std::filesystem::path source_file) :
    source_file_{std::move(source_file)},
    storage_{LoadStorage(model, source_file_)},
    storage_index_to_model_state_var_index_map_{CreateStorageIndexToModelStatevarMappingWithWarnings(model, *storage_)}
{}

opyn::FileBackedStorage::FileBackedStorage(const FileBackedStorage&) = default;
opyn::FileBackedStorage::FileBackedStorage(FileBackedStorage&&) noexcept = default;
FileBackedStorage& opyn::FileBackedStorage::operator=(const FileBackedStorage&) = default;
FileBackedStorage& opyn::FileBackedStorage::operator=(FileBackedStorage&&) noexcept = default;
opyn::FileBackedStorage::~FileBackedStorage() noexcept = default;

osc::ClosedInterval<float> opyn::FileBackedStorage::time_range() const
{
    return {static_cast<float>(storage_->getFirstTime()), static_cast<float>(storage_->getLastTime())};
}

void opyn::FileBackedStorage::reload_from_disk(const OpenSim::Model& model)
{
    storage_ = LoadStorage(model, source_file_);
    storage_index_to_model_state_var_index_map_ = CreateStorageIndexToModelStatevarMappingWithWarnings(model, *storage_);
}
