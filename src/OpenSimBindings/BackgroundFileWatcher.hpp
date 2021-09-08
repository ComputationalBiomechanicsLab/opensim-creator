#pragma once

#include <chrono>
#include <filesystem>
#include <functional>

namespace osc {
    class FileWatcherImpl;

    class FilewatchSubscription final {
        FilewatchSubscription(std::shared_ptr<FileWatcherImpl>, int);

    public:
        std::filesystem::path const& getPath() const;
        std::filesystem::file_time_type getLatestModificationTime() const;

    private:
        std::shared_ptr<FileWatcherImpl> m_FileWatcher;
        int m_SubscriptionId;
    };

    class FileModificationNotification final {
    public:
        FileModificationNotification(std::filesystem::path const&, std::filesystem::file_time_type);
        std::filesystem::path const& getPath() const;
        std::filesystem::file_time_type getModificationTime() const;

    private:
        std::filesystem::path m_Path;
        std::filesystem::file_time_type m_ModificationTime;
    };

    class BackgroundFileWatcher final {
    public:
        BackgroundFileWatcher();
        ~BackgroundFileWatcher() noexcept;
        FilewatchSubscription subscribeToModificationNotifications(std::filesystem::path const&,
                                                                   std::function<FileModificationNotification> onModify);

    private:
        std::unique_ptr<FileWatcherImpl> m_Impl;
    };
}
