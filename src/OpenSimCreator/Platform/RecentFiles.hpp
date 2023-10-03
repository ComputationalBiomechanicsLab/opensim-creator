#pragma once

#include <OpenSimCreator/Platform/RecentFile.hpp>

#include <filesystem>
#include <vector>

namespace osc
{
    // recently-opened files, sorted most-recent to least-recent
    class RecentFiles final {
    public:
        RecentFiles();
        RecentFiles(RecentFiles const&) = delete;
        RecentFiles& operator=(RecentFiles const&) = delete;
        RecentFiles& operator=(RecentFiles&&) noexcept = delete;
        ~RecentFiles() noexcept
        {
            sync();
        }

        void push_back(std::filesystem::path const&);

        [[nodiscard]] bool empty() const
        {
            return m_Files.empty();
        }

        std::vector<RecentFile>::const_iterator begin() const
        {
            return m_Files.begin();
        }

        std::vector<RecentFile>::const_iterator end() const
        {
            return m_Files.end();
        }

        void sync();
    private:
        std::filesystem::path m_DiskLocation;
        std::vector<RecentFile> m_Files;
    };
}
