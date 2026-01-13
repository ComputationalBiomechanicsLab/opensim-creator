#pragma once

#include <liboscar/platform/events/Event.h>

#include <filesystem>
#include <utility>

namespace osc
{
    class OpenFileEvent : public Event {
    public:
        explicit OpenFileEvent(std::filesystem::path path) :
            m_Path{std::move(path)}
        {
            enable_propagation();
        }

        const std::filesystem::path& path() { return m_Path; }
    private:
        std::filesystem::path m_Path;
    };
}
