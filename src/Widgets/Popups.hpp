#pragma once

#include <memory>
#include <vector>

namespace osc { class Popup; }

namespace osc
{
    // generic storage for a drawable popup stack
    class Popups final {
    public:
        Popups();
        Popups(Popups const&) = delete;
        Popups(Popups&&) noexcept;
        Popups& operator=(Popups const&) = delete;
        Popups& operator=(Popups&&) noexcept;
        ~Popups() noexcept;

        void push_back(std::unique_ptr<Popup>);
        void draw();
    private:
        std::vector<std::unique_ptr<Popup>> m_Popups;
    };
}