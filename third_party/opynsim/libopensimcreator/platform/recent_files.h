#pragma once

#include <libopensimcreator/platform/recent_file.h>

#include <filesystem>
#include <vector>

namespace osc
{
    // recently-opened files, sorted most-recent to least-recent
    class RecentFiles final {
    public:
        RecentFiles();
        explicit RecentFiles(std::filesystem::path recent_files_file);
        RecentFiles(RecentFiles&&) noexcept = default;
        RecentFiles(const RecentFiles&) = delete;
        RecentFiles& operator=(const RecentFiles&) = delete;
        RecentFiles& operator=(RecentFiles&&) noexcept = delete;
        ~RecentFiles() noexcept;

        void push_back(const std::filesystem::path&);

        [[nodiscard]] bool empty() const { return files_.empty(); }

        std::vector<RecentFile>::const_iterator begin() const { return files_.begin(); }

        std::vector<RecentFile>::const_iterator end() const { return files_.end(); }

        void sync();

    private:
        std::filesystem::path disk_location_;
        std::vector<RecentFile> files_;
    };
}
