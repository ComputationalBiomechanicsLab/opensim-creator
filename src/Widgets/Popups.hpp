#pragma once

#include <memory>
#include <vector>

namespace osc { class Popup; }

namespace osc
{
    // generic storage for a drawable popup stack
    class Popups final {
    public:
        void draw();
    private:
        std::vector<std::unique_ptr<Popup>> m_Popups;
    };
}