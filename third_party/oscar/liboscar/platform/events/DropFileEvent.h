#pragma once

#include <liboscar/platform/events/Event.h>
#include <liboscar/platform/events/EventType.h>

#include <filesystem>
#include <utility>

namespace osc
{
    // Represents a file that has been dragged into the application.
    class DropFileEvent final : public Event {
    public:
        explicit DropFileEvent(std::filesystem::path path) :
            Event{EventType::DropFile},
            path_{std::move(path)}
        {}

        const std::filesystem::path& path() const { return path_; }

    private:
        std::filesystem::path path_;
    };
}
