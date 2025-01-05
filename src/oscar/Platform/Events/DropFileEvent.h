#pragma once

#include <oscar/Platform/Events/Event.h>

#include <filesystem>

union SDL_Event;

namespace osc
{
    class DropFileEvent final : public Event {
    public:
        explicit DropFileEvent(const SDL_Event&);

        const std::filesystem::path& path() const { return path_; }

    private:
        std::filesystem::path path_;
    };
}
