#include "file_backed_storage.h"

#include <libopynsim/utilities/open_sim_helpers.h>
#include <OpenSim/Common/Storage.h>

#include <filesystem>
#include <memory>
#include <utility>

using namespace opyn;

opyn::FileBackedStorage::FileBackedStorage(const OpenSim::Model& model, std::filesystem::path source_file) :
    m_SourceFile{std::move(source_file)},
    m_Storage{LoadStorage(model, m_SourceFile)},
    m_StorageIndexToModelStateVarIndexMap{CreateStorageIndexToModelStatevarMappingWithWarnings(model, *m_Storage)}
{}

opyn::FileBackedStorage::FileBackedStorage(const FileBackedStorage&) = default;
opyn::FileBackedStorage::FileBackedStorage(FileBackedStorage&&) noexcept = default;
FileBackedStorage& opyn::FileBackedStorage::operator=(const FileBackedStorage&) = default;
FileBackedStorage& opyn::FileBackedStorage::operator=(FileBackedStorage&&) noexcept = default;
opyn::FileBackedStorage::~FileBackedStorage() noexcept = default;

osc::ClosedInterval<float> opyn::FileBackedStorage::time_range() const
{
    return {static_cast<float>(m_Storage->getFirstTime()), static_cast<float>(m_Storage->getLastTime())};
}

void opyn::FileBackedStorage::reload_from_disk(const OpenSim::Model& model)
{
    m_Storage = LoadStorage(model, m_SourceFile);
    m_StorageIndexToModelStateVarIndexMap = CreateStorageIndexToModelStatevarMappingWithWarnings(model, *m_Storage);
}
