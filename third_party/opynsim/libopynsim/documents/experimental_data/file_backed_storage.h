#pragma once

#include <liboscar/maths/closed_interval.h>
#include <liboscar/utilities/clone_ptr.h>

#include <filesystem>
#include <unordered_map>

namespace OpenSim { class Model; }
namespace OpenSim { class Storage; }

namespace opyn
{
    // an `OpenSim::Storage` that's backed by an on-disk file.
    class FileBackedStorage final {
    public:
        explicit FileBackedStorage(const OpenSim::Model&, std::filesystem::path source_file);
        FileBackedStorage(const FileBackedStorage&);
        FileBackedStorage(FileBackedStorage&&) noexcept;
        FileBackedStorage& operator=(const FileBackedStorage&);
        FileBackedStorage& operator=(FileBackedStorage&&) noexcept;
        ~FileBackedStorage() noexcept;

        void reload_from_disk(const OpenSim::Model&);

        osc::ClosedInterval<float> time_range() const;
        const OpenSim::Storage& storage() const { return *storage_; }
        const std::unordered_map<int, int>& mapper() const { return storage_index_to_model_state_var_index_map_; }
    private:
        std::filesystem::path source_file_;
        osc::ClonePtr<OpenSim::Storage> storage_;
        std::unordered_map<int, int> storage_index_to_model_state_var_index_map_;
    };
}
