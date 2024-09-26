#pragma once

#include <oscar/Utils/ClonePtr.h>

#include <filesystem>
#include <memory>
#include <unordered_map>

namespace OpenSim { class Model; }
namespace OpenSim { class Storage; }

namespace osc
{
    // an `OpenSim::Storage` that's backed by an on-disk file.
    class FileBackedStorage final {
    public:
        explicit FileBackedStorage(const OpenSim::Model&, std::filesystem::path sourceFile);
        FileBackedStorage(const FileBackedStorage&);
        FileBackedStorage(FileBackedStorage&&) noexcept;
        FileBackedStorage& operator=(const FileBackedStorage&);
        FileBackedStorage& operator=(FileBackedStorage&&) noexcept;
        ~FileBackedStorage() noexcept;

        void reloadFromDisk(const OpenSim::Model&);

        const OpenSim::Storage& storage() const { return *m_Storage; }
        const std::unordered_map<int, int>& mapper() const { return m_StorageIndexToModelStateVarIndexMap; }
    private:
        std::filesystem::path m_SourceFile;
        ClonePtr<OpenSim::Storage> m_Storage;
        std::unordered_map<int, int> m_StorageIndexToModelStateVarIndexMap;
    };
}
