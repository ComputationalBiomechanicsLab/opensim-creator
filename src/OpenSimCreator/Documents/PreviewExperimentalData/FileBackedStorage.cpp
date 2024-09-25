#include "FileBackedStorage.h"

#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Simulation/Model/Model.h>

#include <filesystem>
#include <memory>
#include <utility>

osc::FileBackedStorage::FileBackedStorage(const OpenSim::Model& model, std::filesystem::path sourceFile) :
    m_SourceFile{std::move(sourceFile)},
    m_Storage{LoadStorage(model, m_SourceFile)},
    m_StorageIndexToModelStateVarIndexMap{CreateStorageIndexToModelStatevarMappingWithWarnings(model, *m_Storage)}
{}

osc::FileBackedStorage::FileBackedStorage(const FileBackedStorage&) = default;
osc::FileBackedStorage::FileBackedStorage(FileBackedStorage&&) noexcept = default;
osc::FileBackedStorage& osc::FileBackedStorage::operator=(const FileBackedStorage&) = default;
osc::FileBackedStorage& osc::FileBackedStorage::operator=(FileBackedStorage&&) noexcept = default;
osc::FileBackedStorage::~FileBackedStorage() noexcept = default;

void osc::FileBackedStorage::reloadFromDisk(const OpenSim::Model& model)
{
    m_Storage = LoadStorage(model, m_SourceFile);
    m_StorageIndexToModelStateVarIndexMap = CreateStorageIndexToModelStatevarMappingWithWarnings(model, *m_Storage);
}
